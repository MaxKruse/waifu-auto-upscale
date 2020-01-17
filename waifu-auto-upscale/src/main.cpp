#include "pch.h"
#define cimg_use_png
#define cimg_use_jpg
#define cimg_use_jpeg

#include <CImg.h>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <sstream>
#include <string>
#define CHECK_F(expected, message) if(!(expected)) { std::cout << (message); exit(1); } 

static std::vector<std::filesystem::path> PrepareFiles(const std::string& root, int resolution)
{
	std::vector<std::filesystem::path> temp;
	temp.reserve(500);
	std::cout << "Preparing files..." << std::endl;
	std::array<std::string, 3> fileTypes = { ".png", ".jpg", ".jpeg" };

	const std::locale locale("");
	typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;
	const converter_type& converter = std::use_facet<converter_type>(locale);

	for (const auto& entry : std::filesystem::directory_iterator(root))
	{
		// Skip Directories
		if (!is_directory(entry.path()))
		{
			auto ws = entry.path().wstring();
			
			std::vector<char> to(ws.length() * converter.max_length());
			std::mbstate_t state;
			const wchar_t* from_next;
			char* to_next;
			const converter_type::result result = converter.out(state, ws.data(), ws.data() + ws.length(), from_next, &to[0], &to[0] + to.size(), to_next);
			if (result != converter_type::ok && result != converter_type::noconv) {
				continue;
			}


			// See: https://stackoverflow.com/a/23658737
			// We find a valid file
			auto found = std::find(fileTypes.begin(), fileTypes.end(), entry.path().extension());
			if(found != fileTypes.end())
			{
				// Open with CImg to check if we need to upscale it.
				auto path = entry.path().string();
				try
				{
					// need height to be 1080 or width to be 1920
					auto image = cimg_library::CImg<unsigned char>(path.c_str());
					
					if (image.width() < (resolution * 16 / 9) && image.height() < resolution)
					{
						temp.emplace_back(entry.path());
					}
				}
				catch (cimg_library::CImgIOException e)
				{
					std::cout << "error: " << e.what();
				}
			}
		}
	}
	std::cout << "Done preparing files. Found: " << temp.size() << " images to scale." <<  std::endl;
	return temp;
}

static void UpscaleIntoFolder(const std::string& exe, const std::string& folder, std::vector<std::filesystem::path>& tempFiles, int resolution)
{
	std::filesystem::current_path(std::filesystem::path(exe).parent_path());
	size_t done = 0;
	for(const auto& entry : tempFiles)
	{
		double scale = 0.1f;
		{
			std::string path = entry.string();
			auto image = cimg_library::CImg<unsigned char>(path.c_str());

			int targetWidth = resolution * 16 / 9;

			bool useHeight = (resolution - image.height()) < (targetWidth - image.width());
			if(useHeight)
			{
				scale += static_cast<double>(resolution) / static_cast<double>(image.height());
			}
			else
			{
				scale += static_cast<double>(targetWidth) / static_cast<double>(image.width());
			}
		}

		std::ostringstream streamObj;
		//Add double to stream
		streamObj << scale;

		std::string command = std::filesystem::path(exe).filename().string() + " -t 1 -p cpu -m noise_scale -n 1 -b 2 -c 32 -s " + streamObj.str(); +"";
		command += " -i \"" + entry.string() + "\"";
		command += " -o \"" + folder + "/" + entry.filename().string() + "\"";
		command += " --model_dir \"" + std::filesystem::path(exe).parent_path().string() + "/models/photo\"";
		command += " > nul";

#ifdef _DEBUG
		std::cout << "Scale for \"" + entry.filename().string() + "\": " + std::to_string(scale) << std::endl;;

		std::cout << "Full command: " << std::endl << command << std::endl;
#endif

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

		system(command.c_str());
		done++;
	}

	std::cout << "Finished scaling all images." << std::endl;
}


int main(int argc, char** args)
{
	setlocale(LC_ALL, "");

	std::cout << "This program will upscale and replace all supported images (png, jpg, jpeg) that are smaller than 1080pixels in height or 1920pixels in width." << std::endl;

	char const* filter[1] = { "*.exe" };
	const char* temp = nullptr;
	
	temp = tinyfd_openFileDialog("Select waifu2x-caffe-cui.exe", "", 1, filter, NULL, 0);
	CHECK_F(temp != NULL, "Aborted waifu2x-caffe-cui.exe selection.");
	std::string exe(temp);

	temp = tinyfd_selectFolderDialog("Select folder of images to upscale", "");
	CHECK_F(temp != NULL, "Aborted folder selection.");
	std::string folder(temp);

	temp = tinyfd_inputBox("Minimum resolution", "What should be the minimum Resolution (eg. 720 for 720p, 1080 for 1080p)", "720");
	CHECK_F(temp != NULL, "No Resolution given.");

	std::string res(temp);

	int resolution = std::stoi(temp);
	CHECK_F(resolution > 0, "Resolution input under 0 given.");

	auto files = PrepareFiles(folder, resolution);

	UpscaleIntoFolder(exe, folder, files, resolution);

	return 0;
}
