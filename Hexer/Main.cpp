#include <SDL.h>
#include <TBRLGPT.h>

using namespace TBRLGPT;

std::string programName = "Hexer";
std::string programVersion = "v0.1";
std::string programTitle = programName + " " + programVersion;
std::string settingsFile = "hexer.ini";

bool programRunning = true;

Environment* e = NULL;
Graphics* g = NULL;
TextPrinter* p = NULL;

int defaultBackColor = 0x101010;
int defaultTextColor = 0xe0e0e0;
int addrForeColor = 0x808080;
int addrBackColor = defaultBackColor;
int bytesForeColor = 0xf0f0f0;
int bytesBackColor = defaultBackColor;
int charsForeColor = 0x808080;
int charsBackColor = defaultBackColor;

unsigned char* file = NULL;
int fileLength = -1;
std::string filename;
std::string currentFileFolder;

struct Bookmark {
	std::string name;
	int start;
	int end;
	int foreColor;
	int backColor;
};

std::vector<Bookmark> bookmarks;

int topAddress = 0;
int maxLines = 32;
int bytesPerLine = 16;

void initSettings() {
	if (!File::Exists(settingsFile))
		return;

	auto settings = File::ReadLines(settingsFile);
	currentFileFolder = String::Trim(settings[0]);
}

void initWindow() {
	e = new Environment(640, 320, 2, false);
	g = e->Gr;
	p = e->Prn;
	g->SetWindowTitle(programTitle);
}

void quitProgram() {
	delete e;
	delete file;
}

void setColors(int foreColor, int backColor) {
	p->SetColor(foreColor, backColor);
	e->Ui->SetColor(foreColor, backColor);
}

void loadFile(std::string path) {
	fileLength = -1;
	file = File::Read(path, &fileLength);
	filename = path;
	topAddress = 0;
	currentFileFolder = File::GetParentDirectory(path);
	bookmarks.clear(); // todo: load bookmarks
}

void selectFile() {
	std::string path = "";
	setColors(defaultTextColor, defaultBackColor);
	FileSelector* fs = new FileSelector(e->Ui);
	path = fs->Select("Select file", currentFileFolder);
	if (path != "") {
		loadFile(path);
	}
}

void drawUi() {
	setColors(defaultTextColor, defaultBackColor);
	p->Clear();

	Window* wTitle = new Window(e->Ui, 0, 0, g->Cols - 2, 1);
	wTitle->Draw();
	
	int maxFilenameLength = g->Cols - 12;
	std::string name = File::GetName(filename);
	std::string abbrName = name.substr(0, maxFilenameLength - 6) + "...";
	wTitle->Print(1, 0, name.length() < maxFilenameLength ? name : abbrName);

	std::string range = String::Format("0x%08X", fileLength - 1);
	wTitle->Print(g->Cols - range.length() - 3, 0, range);

	Window* wBottom = new Window(e->Ui, 0, 36, g->Cols - 2, 2);
	wBottom->Draw();

	delete wTitle;
	delete wBottom;
}

void addBookmark(std::string name, int start, int end, int foreColor, int backColor) {
	Bookmark b;
	b.name = name;
	b.start = start;
	b.end = end;
	b.foreColor = foreColor;
	b.backColor = backColor;
	bookmarks.push_back(b);
}

Bookmark* getBookmark(int address) {
	for (int i = 0; i < bookmarks.size(); i++) {
		Bookmark* b = &bookmarks[i];
		if (address >= b->start && address <= b->end)
			return b;
	}
	return NULL;
}

void printCurrentView() {
	int lineX = 4;
	int firstLineY = 4;
	int address = topAddress;

	p->Locate(lineX + 9, firstLineY - 1);
	setColors(addrForeColor, addrBackColor);
	p->Print("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");

	p->Locate(lineX, firstLineY);
	for (int line = 0; line < maxLines; line++) {
		setColors(addrForeColor, addrBackColor);
		p->Print(String::Format("%08X ", address));
		for (int offset = 0; offset < bytesPerLine; offset++) {
			int ptr = address + offset;
			Bookmark* b = getBookmark(ptr);
			if (b != NULL) {
				setColors(b->foreColor, b->backColor);
			}
			else {
				setColors(bytesForeColor, bytesBackColor);
			}
			p->Print(String::Format("%02X ", file[ptr]));
		}
		for (int offset = 0; offset < bytesPerLine; offset++) {
			int ptr = address + offset;
			Bookmark* b = getBookmark(ptr);
			if (b != NULL) {
				setColors(b->foreColor, b->backColor);
			}
			else {
				setColors(charsForeColor, charsBackColor);
			}
			p->PutChar(file[ptr]);
		}
		p->Locate(lineX, p->GetCursorY() + 1);
		address += bytesPerLine;
	}

	g->Update();
}

void printHelpCommand(std::string key, std::string command) {
	p->SetColor(0x808080, defaultBackColor);
	p->Locate(2, p->GetCursorY());
	p->Print(key);
	p->SetColor(defaultTextColor, defaultBackColor);
	p->Locate(12, p->GetCursorY());
	p->Print(command + "\n");
}

void showHelp() {
	setColors(defaultTextColor, defaultBackColor);
	p->Clear();

	Window* wTitle = new Window(e->Ui, 0, 0, g->Cols - 2, 1);
	wTitle->Draw();
	wTitle->Print(1, 0, "Help");
	wTitle->Print(g->Cols - programTitle.length() - 3, 0, programTitle);

	p->Locate(1, 4);
	printHelpCommand("F1", "Help");
	printHelpCommand("CTRL+Q", "Quit");
	printHelpCommand("CTRL+O", "Open file");
	printHelpCommand("ESC", "Cancel");
	printHelpCommand("DOWN", "Scroll down / cursor down");
	printHelpCommand("UP", "Scroll up / cursor up");
	printHelpCommand("RIGHT", "Cursor right");
	printHelpCommand("LEFT", "Cursor left");
	printHelpCommand("PGDOWN", "Scroll to next page");
	printHelpCommand("PGUP", "Scroll to previous page");
	printHelpCommand("HOME", "Scroll to first address");
	printHelpCommand("END", "Scroll to last address");
	printHelpCommand("ALT+ENTER", "Toggle fullscreen");
	//printHelpCommand("CTRL+E", "Toggle edit mode");
	//printHelpCommand("CTRL+B", "Bookmarks");
	
	Window* wBottom = new Window(e->Ui, 0, 36, g->Cols - 2, 2);
	wBottom->Draw();
	wBottom->Print(1, 0, "Press any key to return...");

	g->Update();
	Key::WaitAny();
	delete wTitle;
	delete wBottom;
}

void keyPressed(SDL_Keycode key) {
	switch (key) {
	case SDLK_DOWN:
		if (topAddress < fileLength - maxLines * bytesPerLine) {
			topAddress += bytesPerLine;
		}
		break;
	case SDLK_UP:
		if (topAddress > 0) {
			topAddress -= bytesPerLine;
		}
		break;
	case SDLK_PAGEDOWN:
		if (topAddress < (fileLength - maxLines * bytesPerLine) - (maxLines * bytesPerLine)) {
			topAddress += maxLines * bytesPerLine;
		}
		else {
			topAddress = fileLength - maxLines * bytesPerLine;
		}
		break;
	case SDLK_PAGEUP:
		if (topAddress > maxLines * bytesPerLine) {
			topAddress -= maxLines * bytesPerLine;
		}
		else {
			topAddress = 0;
		}
		break;
	case SDLK_HOME:
		topAddress = 0;
		break;
	case SDLK_END:
		topAddress = fileLength - maxLines * bytesPerLine;
		break;
	case SDLK_F1:
		showHelp();
		break;
	case SDLK_q:
		if (Key::Ctrl()) {
			programRunning = false;
		}
		break;
	case SDLK_o:
		if (Key::Ctrl()) {
			selectFile();
		}
		break;
	case SDLK_RETURN:
		if (Key::Alt()) {
			g->ToggleFullscreen();
		}
		break;
	default:
		break;
	}
}

int main(int argc, char* args[]) {
	initSettings();
	initWindow();
	selectFile();
	//addBookmark("test", 0x103, 0x11a, 0xff8000, defaultBackColor);
	
	while (file != NULL && programRunning) {
		drawUi();
		printCurrentView();

		SDL_Event e = { 0 };
		SDL_PollEvent(&e);
		if (e.type == SDL_QUIT) {
			programRunning = false;
			break;
		}
		else if (e.type == SDL_KEYDOWN) {
			keyPressed(e.key.keysym.sym);
		}
	}
	
	quitProgram();
	return 0;
}
