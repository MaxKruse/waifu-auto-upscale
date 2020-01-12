#include "pch.h"
#define cimg_use_png
#define cimg_use_jpg
#define cimg_use_jpeg

#include <CImg.h>
#include <tinyfiledialogs/tinyfiledialogs.h>

#define CHECK_F(expected, message) if(!(expected)) { std::cout << message; exit(1); } 

static std::vector<std::filesystem::path> PrepareFiles(const std::string& root)
{
	std::vector<std::filesystem::path> temp;
	temp.reserve(500);
	std::cout << "Preparing files..." << std::endl;
	std::array<std::string, 3> fileTypes = { ".png", ".jpg", ".jpeg" };

	for (const auto& entry : std::filesystem::directory_iterator(root))
	{
		// Skip Directories
		if (!is_directory(entry.path()))
		{
			// See: https://stackoverflow.com/a/23658737
			// We find a valid file
			auto found = std::find(fileTypes.begin(), fileTypes.end(), entry.path().extension());
			if(found != fileTypes.end())
			{
				// Open with CImg to check if we need to upscale it.
				std::string path = entry.path().string();
				auto image = cimg_library::CImg<unsigned char>(path.c_str());

				// need height to be 1080 or width to be 1920

				if(image.width() < 1920 && image.height() < 1080)
				{
					temp.emplace_back(entry.path());
				}
			}
		}
	}
	std::cout << "Done preparing files. Found: " << temp.size() << " images to scale." <<  std::endl;
	return temp;
}

static void UpscaleIntoFolder(const std::string& exe, const std::string& folder, std::vector<std::filesystem::path>& tempFiles)
{
	std::filesystem::current_path(std::filesystem::path(exe).parent_path());
	size_t done = 0;
	for(const auto& entry : tempFiles)
	{
		double scale = 0.0f;
		{
			std::string path = entry.string();
			auto image = cimg_library::CImg<unsigned char>(path.c_str());
			bool useHeight = (1080 - image.height()) < (1920 - image.width());
			if(useHeight)
			{
				scale = 1080.0f / static_cast<double>(image.height());
			}
			else
			{
				scale = 1920.0f / static_cast<double>(image.width());
			}
		}
		std::string command = std::filesystem::path(exe).filename().string() + " -t 1 -p cpu -m noise_scale -n 2 -b 2 -c 64 -s " + std::to_string(scale) + "";
		command += " -i \"" + entry.string() + "\"";
		command += " -o \"" + folder + "/" + entry.filename().string() + "\"";
		command += " --model_dir \"" + std::filesystem::path(exe).parent_path().string() + "/models/photo\"";

		std::cout << "Scale for \"" + entry.filename().string() + "\": " + std::to_string(scale) << std::endl;;

		command += " > nul";

		std::cout << "Full command: " << std::endl << command << std::endl;

		system(command.c_str());
		
		int barWidth = 70;
		float progress = done / static_cast<float>(tempFiles.size());

		std::cout << "[";
		int pos = barWidth * progress;
		for (int i = 0; i < barWidth; ++i) {
			if (i < pos) std::cout << "=";
			else if (i == pos) std::cout << ">";
			else std::cout << " ";
		}
		std::cout << "] " << int(progress * 100.0) << " %\r";
		std::cout.flush();

		done++;

	}

	std::cout << "Finished scaling all images." << std::endl;
}


int main(int argc, char** args)
{

	std::cout << "This program will upscale and replace all supported images (png, jpg, jpeg) that are smaller than 1080pixels in height or 1920pixels in width." << std::endl;

	char const* filter[1] = { "*.exe" };
	const char* temp = nullptr;
	
	temp = tinyfd_openFileDialog("Select waifu2x-caffe-cui.exe", "", 1, filter, NULL, 0);
	CHECK_F(temp != NULL, "Aborted waifu2x-caffe-cui.exe selection");
	std::string exe(temp);

	temp = tinyfd_selectFolderDialog("Select folder of images to upscale", "");
	CHECK_F(temp != NULL, "Aborted folder selection");
	std::string folder(temp);

	auto files = PrepareFiles(folder);

	UpscaleIntoFolder(exe, folder, files);

	return 0;
}
