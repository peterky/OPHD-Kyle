#include "FileIo.h"

#include "../Constants/Strings.h"
#include "../Constants/UiConstants.h"
#include "../ShellOpenPath.h"

#include "MessageBox.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Filesystem.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/EnumKeyCode.h>
#include <NAS2D/Math/Point.h>
#include <NAS2D/Resource/Font.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>


namespace
{
	constexpr int MarginTight{2};

	std::string formatFileTime(const std::filesystem::file_time_type& fileTime)
	{
		const auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		const auto time = std::chrono::system_clock::to_time_t(systemTime);

		std::tm localTime{};
		localtime_s(&localTime, &time);

		std::ostringstream stream;
		stream << std::put_time(&localTime, "%Y-%m-%d %H:%M");
		return stream.str();
	}


	struct SaveGameEntry
	{
		std::string name;
		std::filesystem::file_time_type modifiedTime;
	};
}


int SaveGameListItem::ListBoxTheme::itemHeight() const
{
	return font.height() + MarginTight;
}


void SaveGameListItem::draw(NAS2D::Renderer& renderer, NAS2D::Rectangle<int> drawRect, const ListBoxTheme& theme, bool /*isSelected*/, bool isHighlighted) const
{
	if (isHighlighted)
	{
		renderer.drawBox(drawRect, theme.itemBorderColorMouseHover);
	}

	const auto textColor = isHighlighted ? theme.textColorMouseHover : theme.textColorNormal;
	const auto textMargin = NAS2D::Vector{MarginTight, 0};
	const auto textPosition = drawRect.position + textMargin;

	renderer.drawTextShadow(theme.font, saveName, textPosition, {1, 1}, textColor, NAS2D::Color::Black);

	const auto timestampWidth = theme.font.width(timestamp);
	const auto timestampPosition = NAS2D::Point{drawRect.endPoint().x - timestampWidth - MarginTight, textPosition.y};
	renderer.drawTextShadow(theme.font, timestamp, timestampPosition, {1, 1}, textColor, NAS2D::Color::Black);
}


FileIo::FileIo(FileLoadDelegate fileLoadHandler, FileSaveDelegate fileSaveHandler) :
	Window{"File I/O"},
	mFileLoadHandler{fileLoadHandler},
	mFileSaveHandler{fileSaveHandler},
	mMode{FileOperation::Load},
	mOpenSaveFolder{"Open Save Folder", {this, &FileIo::onOpenFolder}},
	mCancel{"Cancel", {this, &FileIo::onClose}},
	mFileOperation{"FileOp", {this, &FileIo::onFileIo}},
	mDeleteFile{"Delete", {this, &FileIo::onFileDelete}},
	mSortByName{"Name", {this, &FileIo::onSortByName}},
	mSortByDate{"Date", {this, &FileIo::onSortByDate}},
	mFileName{50, {this, &FileIo::onFileNameChange}},
	mListBox{{this, &FileIo::onFileSelect}}
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseDoubleClick().connect({this, &FileIo::onDoubleClick});
	eventHandler.keyDown().connect({this, &FileIo::onKeyDown});

	mSortByName.size({50, 18});
	mSortByDate.size({50, 18});
	add(mSortByName, {5, sWindowTitleBarHeight + 2});
	add(mSortByDate, {60, sWindowTitleBarHeight + 2});
	updateSortButtons();

	mOpenSaveFolder.size({std::max(105, mOpenSaveFolder.size().x + constants::Margin), 20});
	add(mOpenSaveFolder, {5 + 690 - mOpenSaveFolder.size().x, sWindowTitleBarHeight + 2});

	mListBox.size({690, 253});
	mListBox.visible(true);
	add(mListBox, {5, 45});

	mFileName.size({mListBox.size().x, std::max(18, mFileName.size().y)});
	add(mFileName, mListBox.area().crossYPoint() - NAS2D::Point{0, 0} + NAS2D::Vector{0, 4});

	const auto bottomButtonArea = NAS2D::Rectangle{mFileName.area().crossYPoint() + NAS2D::Vector{0, 5}, {mFileName.size().x, 20}};

	mFileOperation.size({50, 20});
	mFileOperation.enabled(false);
	add(mFileOperation, {bottomButtonArea.endPoint().x - mFileOperation.size().x, bottomButtonArea.position.y});

	mDeleteFile.size({std::max(50, mDeleteFile.size().x + constants::Margin), 20});
	mDeleteFile.enabled(false);
	add(mDeleteFile, {bottomButtonArea.position.x, bottomButtonArea.position.y});

	mCancel.size({std::max(50, mCancel.size().x + constants::Margin), 20});
	add(mCancel, {mFileOperation.position().x - mCancel.size().x - 5, bottomButtonArea.position.y});

	size(bottomButtonArea.endPoint() - NAS2D::Point{0, 0} + NAS2D::Vector{5, 5});
}


FileIo::~FileIo()
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseDoubleClick().disconnect({this, &FileIo::onDoubleClick});
	eventHandler.keyDown().disconnect({this, &FileIo::onKeyDown});
}


void FileIo::showOpen(const std::string& directory)
{
	scanDirectory(directory);
	mMode = FileOperation::Load;
	title(constants::WindowFileIoTitleLoad);
	mFileOperation.text(constants::WindowFileIoLoad);
	show();
}


void FileIo::showSave(const std::string& directory)
{
	scanDirectory(directory);
	mMode = FileOperation::Save;
	title(constants::WindowFileIoTitleSave);
	mFileOperation.text(constants::WindowFileIoSave);
	show();
}


void FileIo::scanDirectory(const std::string& directory)
{
	mDirectory = directory;

	const auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
	mScanPath = (filesystem.prefPath() / NAS2D::VirtualPath{directory}).string();

	auto dirList = filesystem.directoryList(NAS2D::VirtualPath{directory});
	const auto saveDirectory = std::filesystem::path{filesystem.prefPath().string()} / directory;

	std::vector<SaveGameEntry> entries;
	entries.reserve(dirList.size());

	for (const auto& dir : dirList)
	{
		if (filesystem.isDirectory(NAS2D::VirtualPath{directory + dir.string()}))
		{
			continue;
		}

		const auto diskPath = saveDirectory / dir.string();
		entries.push_back({dir.stem().string(), std::filesystem::last_write_time(diskPath)});
	}

	if (mSortMode == SortMode::Name)
	{
		std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
			return left.name < right.name;
		});
	}
	else
	{
		std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
			return left.modifiedTime > right.modifiedTime;
		});
	}

	mListBox.clear();
	for (const auto& entry : entries)
	{
		mListBox.add(entry.name, formatFileTime(entry.modifiedTime));
	}
}


void FileIo::updateSortButtons()
{
	mSortByName.enabled(mSortMode != SortMode::Name);
	mSortByDate.enabled(mSortMode != SortMode::DateTime);
}


void FileIo::onSortByName()
{
	mSortMode = SortMode::Name;
	updateSortButtons();
	scanDirectory(mDirectory);
}


void FileIo::onSortByDate()
{
	mSortMode = SortMode::DateTime;
	updateSortButtons();
	scanDirectory(mDirectory);
}


void FileIo::onDoubleClick(NAS2D::MouseButton /*button*/, NAS2D::Point<int> position)
{
	if (!visible()) { return; } // ignore key presses when hidden.

	if (mListBox.area().contains(position))
	{
		if (mListBox.isItemSelected() && !mFileName.isEmpty())
		{
			onFileIo();
		}
	}
}


/**
 * Event handler for Key Down.
 */
void FileIo::onKeyDown(NAS2D::KeyCode key, NAS2D::KeyModifier /*mod*/, bool /*repeat*/)
{
	if (!visible()) { return; } // ignore key presses when hidden.

	if (key == NAS2D::KeyCode::Enter || key == NAS2D::KeyCode::KeypadEnter)
	{
		if (!mFileName.isEmpty())
		{
			onFileIo();
		}
	}
}


void FileIo::onOpenFolder() const
{
	shellOpenPath(mScanPath);
}


void FileIo::onFileSelect()
{
	mFileName.text(mListBox.isItemSelected() ? mListBox.selected().saveName : "");
}


void FileIo::onFileNameChange(TextField& control)
{
	std::string sFile = control.text();

	const std::string RestrictedFilenameChars = "\\/:*?\"<>|";

	if (sFile.empty()) // no blank filename
	{
		mFileOperation.enabled(false);
		mDeleteFile.enabled(false);
	}
	else if (sFile.find_first_of(RestrictedFilenameChars) != std::string::npos)
	{
		mFileOperation.enabled(false);
		mDeleteFile.enabled(false);
	}
	else
	{
		mFileOperation.enabled(true);
		mDeleteFile.enabled(true);
	}
}


void FileIo::close()
{
	visible(false);
	mFileName.clear();
}


void FileIo::onClose()
{
	close();
}


void FileIo::onFileIo()
{
	if(mMode == FileOperation::Save)
	{
		if(mFileSaveHandler) { mFileSaveHandler(mFileName.text()); }
	}

	if(mMode == FileOperation::Load)
	{
		mFileLoadHandler(mFileName.text());
	}
	mFileName.clear();
	mFileOperation.enabled(false);
}


void FileIo::onFileDelete()
{
	std::string filename = constants::SaveGamePath + mFileName.text() + ".xml";

	try
	{
		if(doYesNoMessage(constants::WindowFileIoTitleDelete, "Are you sure you want to delete " + mFileName.text() + "?"))
		{
			NAS2D::Utility<NAS2D::Filesystem>::get().del(NAS2D::VirtualPath{filename});
		}
	}
	catch(const std::exception& e)
	{
		doNonFatalErrorMessage("Delete Failed", e.what());
	}

	mFileName.clear();
	mDeleteFile.enabled(false);
	scanDirectory(constants::SaveGamePath);
}