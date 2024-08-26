#include "include/Renderer.h"

void Renderer::render()
{
	GSceneMetrics.startTimer(Timers::all);
	if (scene->getIsDirty()) throw std::invalid_argument("Scene is dirty, cannot render");

	// Prepare Primary Queue
	GSceneMetrics.startTimer(Timers::generateQueue);

	size_t numBuckets = getNumBuckets();

	queueBuckets.resize(numBuckets);
	for (size_t y = rendererOutput.startY; y < rendererOutput.endY; ++y) {
		for (size_t x = rendererOutput.startX; x < rendererOutput.endX; ++x) {
			size_t bucketId = (y * rendererOutput.width + x) % numBuckets;
			TraceQueue& queue = queueBuckets[bucketId];
			scene->camera.emplaceTask(rendererOutput.width, rendererOutput.height, x, y, queue);
		}
	}

	GSceneMetrics.stopTimer(Timers::generateQueue);
	GSceneMetrics.startTimer(Timers::processQueue);
	launchBuckets();
	GSceneMetrics.stopTimer(Timers::processQueue);
	GSceneMetrics.stopTimer(Timers::all);
}

size_t Renderer::getNumBuckets() const
{
	size_t numPixels = rendererOutput.width * rendererOutput.height;
	size_t numBuckets = numPixels / scene->bucketSize;
	if (numPixels % scene->bucketSize != 0) { ++numBuckets; }
	return numBuckets;
}

void Renderer::launchBuckets()
{
	const size_t numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	std::atomic<size_t> nextQueueIndex{ numThreads };
	GSceneMetrics.reserveThread(numThreads);

	for (size_t threadId = 0; threadId < numThreads; ++threadId) {
		threads.emplace_back(&Renderer::workerThread, this, threadId, std::ref(nextQueueIndex));
	}

	for (auto& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

void Renderer::workerThread(size_t threadIdx, std::atomic<size_t>& nextQueueIndex) {
	GThreadIdx = threadIdx;
	size_t queueIndex = threadIdx;
	while (queueIndex < queueBuckets.size()) {
		processTraceQueue(queueBuckets[queueIndex]);
		queueIndex = nextQueueIndex.fetch_add(1);
	}
}

void Renderer::processTraceQueue(TraceQueue& traceQueue)
{
	while (!traceQueue.empty()) {
		TraceTask task = traceQueue.front();
		Ray& ray = task.ray;
		traceQueue.pop();
		TraceHit hit{};
		scene->intersect(ray, hit);
		processXData(task, hit, traceQueue);
	}
}

void Renderer::processXData(TraceTask& task, TraceHit& hit, TraceQueue& traceQueue) {
	if (task.depth >= settings->maxDepth) {
		shadeSky(task, hit);
		return;
	}

	if (!hit.successful()) {
		shadeSky(task, hit);
		return;
	}

	const auto& material = scene->materials[hit.materialIndex];
	switch (material.type) {
	case Material::Type::DIFFUSE:
		shadeDiffuse(task, hit);
		break;
	case Material::Type::CONSTANT:
		shadeConstant(task, hit);
		break;
	case Material::Type::REFLECTIVE:
		shadeReflective(task, hit, traceQueue);
		break;
	case Material::Type::REFRACTIVE:
		shadeRefractive(task, hit, traceQueue);
		break;
	case Material::Type::DEBUG:
		shadeDebug(task, hit, traceQueue);
		break;
	case Material::Type::DEBUG_NORMAL:
		shadeNormal(task, hit);
		break;
	case Material::Type::DEBUG_UV:
		shadeUv(task, hit);
		break;
	case Material::Type::DEBUG_BARY:
		shadeBary(task, hit);
		break;
	case Material::Type::VOID:
		throw std::invalid_argument("not handled");
		break;
	}
}

void Renderer::shadeSky(const TraceTask& task, const TraceHit& hit)
{
	hit;

	Vec3 skyColor;
	if (!scene->useSkybox) {
		skyColor = scene->bgColor;
	}
	else {
		skyColor = scene->skybox.sample(task.ray.getDirection());
	}

	rendererOutput.addSample(task, skyColor, hit);
}

void Renderer::shadeSkySimple(const TraceTask& task, const TraceHit& hit) {
	hit;
	GSceneMetrics.record("ShadeSky");
	float t = 0.5f * (task.ray.getDirection().y + 1.f);
	Vec3 white{ 1.f, 1.f, 1.f };
	Vec3 blue{ 0.5f, 0.7f, 1.f };
	Vec3 skyColor = lerp(white, blue, t);

	if (task.weight > epsilon) {
		rendererOutput.addSample(task, skyColor, hit, BlendType::ADDITIVE);
	}
}

void Renderer::shadeDebug(TraceTask& task, const TraceHit& hit, TraceQueue& traceQueue)
{
	if (task.weight > epsilon) {
		Vec3 color;
		if (hit.kdtreeIdx == 0) {
			color = { 1.0f, 0.f, 0.f };
		}
		if (hit.kdtreeIdx == 1) {
			color = { 0.0f, 1.f, 0.f };
		}
		if (hit.kdtreeIdx == 2) {
			color = { 0.0f, 0.f, 1.f };
		}

		task.weight *= 0.6f;
		rendererOutput.addSample(task, color, hit);
		task.ray.origin = task.ray.origin + task.ray.getDirection() * (hit.t + 0.5f);
		++task.depth;
		traceQueue.push(task);
	}
}

void Renderer::shadeConstant(TraceTask& task, const TraceHit& hit)
{
	auto& material = scene->materials[hit.materialIndex];
	if (task.weight > epsilon) {
		rendererOutput.addSample(task, material.getAlbedo(*scene, hit), hit);
	}
}

void Renderer::shadeDiffuse(const TraceTask& task, const TraceHit& hit)
{
	if (task.weight < epsilon) {
		return;
	}

	const Material& material = scene->materials[hit.materialIndex];
	Vec3 albedo = material.getAlbedo(*scene, hit);
	Vec3 light = hitLight(hit.biasP(settings->bias), hit.n); // overexposure x,y,z > 1.f

	Vec3 ambient = multiply(scene->ambientLightColor, albedo);
	light += ambient;

	Vec3 overexposedColor = multiply(light, albedo);
	Vec3 diffuseComponent = clampOverexposure(overexposedColor);
	rendererOutput.addSample(task, diffuseComponent, hit, BlendType::NORMAL);
}

void Renderer::shadeReflective(TraceTask& task, const TraceHit& hit, TraceQueue& traceQueue)
{
	auto& material = scene->materials[hit.materialIndex];

	float originalWeight = task.weight;
	task.weight *= material.diffuseness;
	shadeDiffuse(task, hit);

	task.weight = originalWeight * material.reflectivity;
	if (task.weight > epsilon) {
		task.ray.reflect(hit.biasP(settings->bias), hit.n);
		++task.depth;
		traceQueue.push(task);
	}
}

void Renderer::shadeRefractive(TraceTask& task, TraceHit& hit, TraceQueue& traceQueue)
{
#ifndef NDEBUG
	auto oldRay = task.ray;
#endif

	auto& material = scene->materials[hit.materialIndex];
	float etai = 1, etat = material.ior;
	if (hit.type == TraceHitType::INSIDE_REFRACTIVE) {
		hit.n = -hit.n;
		std::swap(etai, etat);
		GSceneMetrics.record("shadeRefractive_INSIDE");
	}
	else {
		GSceneMetrics.record("shadeRefractive_OUTSIDE");
	}

	bool reflected = false, transmitted = false;
	// Reflection
	TraceTask reflectiveTask = task;
	float fresnelFactor = schlickApprox(reflectiveTask.ray.getDirection(), hit.n, etai, etat);

	reflectiveTask.weight *= fresnelFactor;
	if (reflectiveTask.weight > epsilon) {
		reflectiveTask.ray.reflect(hit.biasP(settings->bias), hit.n);
		++reflectiveTask.depth;
		traceQueue.push(reflectiveTask);
		reflected = true;
	}

	// Refraction
	TraceTask& refractionTask = task;
	bool hasRefraction = refractionTask.ray.refractVladi(hit.biasP(-settings->bias), hit.n, etai, etat);
	if (hasRefraction) {

		refractionTask.weight *= (1.f - fresnelFactor);
		if (refractionTask.weight > epsilon) {
			++refractionTask.depth;
			refractionTask.ior = etat;
			traceQueue.push(refractionTask);
			transmitted = true;
		}
	}

	if (!reflected && !transmitted) {
		shadeSky(task, hit);
	}

#ifndef NDEBUG
	// assert refractP is farther away from ray.origin than hit.p
	Vec3 oP = hit.p - oldRay.origin;
	float oPLen = oP.lengthSquared();
	Vec3 oReflect = hit.biasP(settings->bias) - oldRay.origin;
	float oReflectLen = oReflect.lengthSquared();
	assert(oReflectLen < oPLen + 10 * epsilon);
#endif
}

Vec3 Renderer::hitLight(const Vec3& p, const Vec3& n) const
{
	Vec3 shade{ 0.f, 0.f, 0.f };
	for (const Light& light : scene->lights) {
		shade = shade + light.lightContrib(*scene, p, n);
	}

	return shade;
}

inline Vec3 Renderer::clampOverexposure(const Vec3& overexposed) const {
	Vec3 rest = overexposed - Vec3{ 1.f, 1.f, 1.f };
	rest.clamp(0.f, FLT_MAX);

	// Using a logarithmic approach to make the change more gradual
	Vec3 adjusted = Vec3{
		log10(1.0f + rest.x),
		log10(1.0f + rest.y),
		log10(1.0f + rest.z)
	};

	Vec3 result = overexposed - adjusted;
	result.clamp(0.f, 1.f);
	return result;
}

float Renderer::schlickApproxWikipedia(const Vec3& i, const Vec3& n, float n1, float n2) const {
	// assumption: n is always facing i
	float cosTheta = -dot(i, n);
	float F0 = std::powf((n1 - n2) / (n1 + n2), 2);
	return F0 + (1.f - F0) * std::powf(1.f - cosTheta, 5);
}

float Renderer::schlickApproxVladi(const Vec3& i, const Vec3& n) const {
	float cosTheta = dot(i, n);
	return 0.5f * powf(1.f + cosTheta, 5); // vladi's value
}

Color Renderer::traceImagePlane(const Ray& ray) const
{
	float imagePlaneDist = 1.f;
	Vec3 p = ray.origin + ray.getDirection();
	Vec3 ortho = ray.origin + scene->camera.getDir();
	float rayProj = dot(ortho, p);

	float scale = imagePlaneDist / rayProj;
	p = p * scale;
	TraceHit hit;
	hit.p = p;
	return shadePosition(hit);
}

Color Renderer::shadePosition(const TraceHit& hit) const {
	uint8_t r = static_cast<uint8_t>(fabs(hit.p.x * 100.f));
	uint8_t g = static_cast<uint8_t>(fabs(hit.p.y * 100.f));
	uint8_t b = static_cast<uint8_t>(fabs(hit.p.z * 100.f));

	return Color{ r, g, b };
}

Color Renderer::shadePerlin(Vec3 p) const
{
	static const siv::PerlinNoise::seed_type seed = 123456u;
	static const siv::PerlinNoise perlin{ seed };

	const double noise = perlin.octave2D_01(p.x, p.y, 4);
	const uint8_t color = static_cast<uint8_t>(noise * 255);
	return Color{ color, color, color };
}

void Renderer::shadeBary(const TraceTask& task, const TraceHit& hit)
{
	assert(hit.baryU <= 1.f && hit.baryU >= 0);
	assert(hit.baryV <= 1.f && hit.baryV >= 0);
	Vec3 unitColor = { hit.baryU, 0.f, hit.baryV };
	rendererOutput.addSample(task, unitColor, hit);
}

void Renderer::shadeUv(const TraceTask& task, const TraceHit& hit)
{
	assert(hit.u <= 1.f && hit.u >= 0);
	assert(hit.v <= 1.f && hit.u >= 0);
	Vec3 unitColor = { hit.u, 0.f, hit.v };
	rendererOutput.addSample(task, unitColor, hit);
}

void Renderer::shadeNormal(TraceTask& task, const TraceHit& hit)
{
	Vec3 unitColor = hit.n * 0.5f + Vec3{ 0.5f, 0.5f, 0.5f };
	rendererOutput.addSample(task, unitColor, hit);
}
