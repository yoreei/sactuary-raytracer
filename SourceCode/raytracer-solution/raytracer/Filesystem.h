#pragma once

#include<filesystem>

using Path = std::filesystem::path;

namespace Filesystem {
	inline Path addSubExt(Path path, std::string subExt)
	{
		Path stem = path.stem(); // e.g., "filename"
		Path extension = path.extension(); // e.g., ".png"
		
		Path newFilename = stem.string() + subExt + extension.string();
		
		return path.parent_path() / newFilename;
	}
};
