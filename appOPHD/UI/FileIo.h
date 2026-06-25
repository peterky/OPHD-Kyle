#pragma once

#include <libControls/Window.h>
#include <libControls/Button.h>
#include <libControls/TextField.h>
#include <libControls/ListBox.h>

#include <NAS2D/Signal/Delegate.h>
#include <NAS2D/Renderer/Color.h>
#include <NAS2D/Renderer/Renderer.h>

#include <string>


namespace NAS2D
{
	class Font;

	enum class KeyModifier : uint16_t;
	enum class KeyCode : uint32_t;
	enum class MouseButton;

	template <typename BaseType> struct Point;
	template <typename BaseType> struct Rectangle;
}


struct SaveGameListItem
{
	std::string saveName;
	std::string timestamp;

	struct ListBoxTheme
	{
		const NAS2D::Font& font;

		NAS2D::Color borderColorNormal = NAS2D::Color{75, 75, 75};
		NAS2D::Color borderColorActive = NAS2D::Color{0, 185, 0};

		NAS2D::Color itemBorderColorMouseHover = NAS2D::Color::DarkGreen;

		NAS2D::Color backgroundColorNormal = NAS2D::Color{0, 85, 0, 220};
		NAS2D::Color backgroundColorSelected = NAS2D::Color{0, 100, 0, 231};

		NAS2D::Color textColorNormal = NAS2D::Color::White;
		NAS2D::Color textColorMouseHover = NAS2D::Color::White;

		int itemHeight() const;
	};

	void draw(NAS2D::Renderer& renderer, NAS2D::Rectangle<int> itemDrawRect, const ListBoxTheme& theme, bool isSelected, bool isHighlighted) const;
};


class FileIo : public Window
{
public:
	using FileLoadDelegate = NAS2D::Delegate<void(const std::string&)>;
	using FileSaveDelegate = NAS2D::Delegate<void(const std::string&)>;

	FileIo(FileLoadDelegate fileLoadHandler, FileSaveDelegate fileSaveHandler = {});
	~FileIo() override;

	void showOpen(const std::string& directory);
	void showSave(const std::string& directory);
	void close();

protected:
	enum class FileOperation
	{
		Load,
		Save
	};

	enum class SortMode
	{
		Name,
		DateTime
	};

	void scanDirectory(const std::string& directory);
	void updateSortButtons();

	void onDoubleClick(NAS2D::MouseButton button, NAS2D::Point<int> position);
	void onKeyDown(NAS2D::KeyCode key, NAS2D::KeyModifier mod, bool repeat);

	void onOpenFolder() const;
	void onFileSelect();
	void onFileNameChange(TextField& control);
	void onClose();
	void onFileIo();
	void onFileDelete();
	void onSortByName();
	void onSortByDate();

private:
	FileLoadDelegate mFileLoadHandler;
	FileSaveDelegate mFileSaveHandler;

	FileOperation mMode;
	SortMode mSortMode{SortMode::Name};

	std::string mScanPath;
	std::string mDirectory;

	Button mOpenSaveFolder;
	Button mCancel;
	Button mFileOperation;
	Button mDeleteFile;
	Button mSortByName;
	Button mSortByDate;

	TextField mFileName;

	ListBox<SaveGameListItem> mListBox;
};