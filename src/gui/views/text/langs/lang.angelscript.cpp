// ---------------------------------------------------------------------------
//
// Retrodev Gui
//
// AngelScript language definition and parser for TextEditor.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------

#include <imgui.text.editor.h>
#include <ctre.hpp>
#include <deque>
#include <filesystem>
#include <string_view>
#include "lang.angelscript.h"

namespace ImGui {
	namespace TextEditorLangs {
		namespace AngelScript {
			//
			// AngelScript reserved keywords
			//
			const char* const kAngelScriptKeywords[] = {
				"and",	  "abstract", "auto",	"bool", "break", "case",	  "cast",	  "class", "const",	 "continue", "default",	 "do",		"double",	 "else",	  "enum",
				"false",  "final",	  "float",	"for",	"from",	 "funcdef",	  "function", "get",   "if",	 "import",	 "in",		 "inout",	"int",		 "interface", "int8",
				"int16",  "int32",	  "int64",	"is",	"mixin", "namespace", "not",	  "null",  "or",	 "out",		 "override", "private", "protected", "return",	  "set",
				"shared", "super",	  "switch", "this", "true",	 "typedef",	  "uint",	  "uint8", "uint16", "uint32",	 "uint64",	 "void",	"while",	 "xor"};
			//
			// AngelScript built-in math/utility functions
			//
			const char* const kAngelScriptIdentifiers[] = {"cos",		  "sin",		 "tab",			"acos",		  "asin",	  "atan",	 "atan2",	 "cosh",
															"sinh",		  "tanh",		 "log",			"log10",	  "pow",	  "sqrt",	 "abs",		 "ceil",
															"floor",	  "fraction",	 "closeTo",		"fpFromIEEE", "fpToIEEE", "complex", "opEquals", "opAddAssign",
															"opSubAssign", "opMulAssign", "opDivAssign", "opAdd",	  "opSub",	  "opMul",	 "opDiv"};
			//
			// Built-in API symbols injected as static codelens entries.
			// Each entry is: { symbolName, signature, description }
			//
			struct ApiSymbolDef {
				const char* name;
				const char* signature;
				const char* description;
			};
			//
			// Global log functions exposed to all scripts
			//
			static const ApiSymbolDef kApiGlobalFunctions[] = {
				{"Log_Info", "void Log_Info(const string &in msg)", "Log an informational message to the script output console."},
				{"Log_Warning", "void Log_Warning(const string &in msg)", "Log a warning message to the script output console."},
				{"Log_Error", "void Log_Error(const string &in msg)", "Log an error message to the script output console."},
			};
			//
			// Image object methods
			//
			static const ApiSymbolDef kApiImageMethods[] = {
				{"GetWidth", "int GetWidth() const", "Returns the width of the image in pixels."},
				{"GetHeight", "int GetHeight() const", "Returns the height of the image in pixels."},
				{"GetPixelColor", "RgbColor GetPixelColor(int x, int y) const", "Returns the RGBA colour of the pixel at (x, y)."},
			};
			//
			// Palette object methods
			//
			static const ApiSymbolDef kApiPaletteMethods[] = {
				{"PaletteMaxColors", "int PaletteMaxColors() const", "Returns the total number of pen slots in the active palette."},
				{"PenGetColor", "RgbColor PenGetColor(int pen) const", "Returns the RGB colour assigned to pen slot index."},
				{"GetSystemIndexByColor", "int GetSystemIndexByColor(const RgbColor &in c, const string &in mode) const",
				 "Returns the system-palette hardware index for a colour in a given mode."},
				{"PenGetColorIndex", "int PenGetColorIndex(int pen) const", "Returns the hardware colour index stored in pen slot index."},
				{"PenGetIndex", "int PenGetIndex(const RgbColor &in c) const", "Returns the pen slot index that matches colour c, or -1 if not found."},
			};
			//
			// BitmapExportContext object methods
			//
			static const ApiSymbolDef kApiBitmapContextMethods[] = {
				{"GetNativeWidth", "int GetNativeWidth() const", "Returns the image width in native hardware units for the active mode."},
				{"GetNativeHeight", "int GetNativeHeight() const", "Returns the image height in native hardware units for the active mode."},
				{"GetTargetMode", "string GetTargetMode() const", "Returns the target video mode name (e.g. \"Mode 0\")."},
				{"GetTargetSystem", "string GetTargetSystem() const", "Returns the target system identifier (e.g. \"amstrad.cpc\")."},
				{"GetParam", "string GetParam(const string &in key) const", "Returns the export parameter value for key, or empty string if absent."},
				{"GetPalette", "Palette@ GetPalette() const", "Returns the active palette object for pen/colour lookup."},
				{"GetUseTransparentColor", "bool GetUseTransparentColor() const", "Returns true when transparent colour handling is enabled for this asset."},
				{"GetTransparentPen", "int GetTransparentPen() const", "Returns the pen index reserved for transparent pixels, or -1 if none is configured."},
			};
			//
			// TilesetExportContext object methods
			//
			static const ApiSymbolDef kApiTilesetContextMethods[] = {
				{"GetTileCount", "int GetTileCount() const", "Returns the total number of tiles in the tileset."},
				{"GetTile", "Image@ GetTile(int index) const", "Returns the Image object for tile at index."},
				{"GetTileWidth", "int GetTileWidth() const", "Returns the tile width in pixels."},
				{"GetTileHeight", "int GetTileHeight() const", "Returns the tile height in pixels."},
				{"GetNativeWidth", "int GetNativeWidth() const", "Returns the tile width in native hardware units."},
				{"GetNativeHeight", "int GetNativeHeight() const", "Returns the tile height in native hardware units."},
				{"GetTargetMode", "string GetTargetMode() const", "Returns the target video mode name."},
				{"GetTargetSystem", "string GetTargetSystem() const", "Returns the target system identifier."},
				{"GetParam", "string GetParam(const string &in key) const", "Returns the export parameter value for key."},
				{"GetPalette", "Palette@ GetPalette() const", "Returns the active palette object."},
				{"GetUseTransparentColor", "bool GetUseTransparentColor() const", "Returns true when transparent colour handling is enabled for this asset."},
				{"GetTransparentPen", "int GetTransparentPen() const", "Returns the pen index reserved for transparent pixels, or -1 if none is configured."},
			};
			//
			// SpriteExportContext object methods
			//
			static const ApiSymbolDef kApiSpriteContextMethods[] = {
				{"GetSpriteCount", "int GetSpriteCount() const", "Returns the total number of sprites."},
				{"GetSprite", "Image@ GetSprite(int index) const", "Returns the Image object for sprite at index."},
				{"GetSpriteWidth", "int GetSpriteWidth(int index) const", "Returns the width in pixels of sprite at index."},
				{"GetSpriteHeight", "int GetSpriteHeight(int index) const", "Returns the height in pixels of sprite at index."},
				{"GetSpriteName", "string GetSpriteName(int index) const", "Returns the name assigned to sprite at index."},
				{"GetNativeWidth", "int GetNativeWidth() const", "Returns the native width for the active mode."},
				{"GetNativeHeight", "int GetNativeHeight() const", "Returns the native height for the active mode."},
				{"GetTargetMode", "string GetTargetMode() const", "Returns the target video mode name."},
				{"GetTargetSystem", "string GetTargetSystem() const", "Returns the target system identifier."},
				{"GetParam", "string GetParam(const string &in key) const", "Returns the export parameter value for key."},
				{"GetPalette", "Palette@ GetPalette() const", "Returns the active palette object."},
				{"GetUseTransparentColor", "bool GetUseTransparentColor() const", "Returns true when transparent colour handling is enabled for this asset."},
				{"GetTransparentPen", "int GetTransparentPen() const", "Returns the pen index reserved for transparent pixels, or -1 if none is configured."},
			};
			//
			// MapExportContext object methods
			//
			static const ApiSymbolDef kApiMapContextMethods[] = {
				{"GetLayerCount", "int GetLayerCount() const", "Returns the number of layers in the map."},
				{"GetLayerName", "string GetLayerName(int layerIndex) const", "Returns the name of layer at layerIndex."},
				{"GetLayerWidth", "int GetLayerWidth(int layerIndex) const", "Returns the column count of layer at layerIndex."},
				{"GetLayerHeight", "int GetLayerHeight(int layerIndex) const", "Returns the row count of layer at layerIndex."},
				{"GetCell", "int GetCell(int layerIndex, int col, int row) const", "Returns the packed cell value at (col, row) in layerIndex."},
				{"GetCellTilesetIndex", "int GetCellTilesetIndex(int cellVal) const", "Extracts the tileset index from a packed cell value."},
				{"GetCellTileIndex", "int GetCellTileIndex(int cellVal) const", "Extracts the tile index within its tileset from a packed cell value."},
				{"GetViewWidth", "int GetViewWidth() const", "Returns the viewport width."},
				{"GetViewHeight", "int GetViewHeight() const", "Returns the viewport height."},
				{"GetParam", "string GetParam(const string &in key) const", "Returns the export parameter value for key."},
			};
			//
			// RgbColor value-type fields and methods
			//
			static const ApiSymbolDef kApiRgbColorFields[] = {
				{"r", "uint8 r", "Red channel (0-255)."},
				{"g", "uint8 g", "Green channel (0-255)."},
				{"b", "uint8 b", "Blue channel (0-255)."},
				{"a", "uint8 a", "Alpha channel (0-255)."},
				{"IsTransparent", "bool IsTransparent() const", "Returns true when the alpha channel is 0 (fully transparent)."},
				{"IsOpaque", "bool IsOpaque() const", "Returns true when the alpha channel is 255 (fully opaque)."},
			};
			//
			// array<T> addon methods (scriptarray)
			//
			static const ApiSymbolDef kApiArrayMethods[] = {
				{"length", "uint length() const", "Returns the number of elements in the array."},
				{"isEmpty", "bool isEmpty() const", "Returns true if the array contains no elements."},
				{"reserve", "void reserve(uint maxElements)", "Pre-allocates memory for maxElements without changing the array length."},
				{"resize", "void resize(uint numElements)", "Sets the length of the array, adding default-constructed or removing elements as needed."},
				{"insertAt", "void insertAt(uint index, const T& in value)", "Inserts value before the element at index."},
				{"insertLast", "void insertLast(const T& in value)", "Appends value to the end of the array."},
				{"removeAt", "void removeAt(uint index)", "Removes the element at index, shifting subsequent elements down."},
				{"removeLast", "void removeLast()", "Removes the last element of the array."},
				{"removeRange", "void removeRange(uint start, uint count)", "Removes count elements starting at start."},
				{"sortAsc", "void sortAsc()", "Sorts all elements in ascending order using the default comparator."},
				{"sortDesc", "void sortDesc()", "Sorts all elements in descending order using the default comparator."},
				{"reverse", "void reverse()", "Reverses the order of all elements in the array."},
				{"find", "int find(const T& in value) const", "Returns the index of the first element equal to value, or -1 if not found."},
				{"findByRef", "int findByRef(const T& in ref) const", "Returns the index of the first element whose address matches ref, or -1 if not found."},
			};
			//
			// dictionary addon methods (scriptdictionary)
			//
			static const ApiSymbolDef kApiDictionaryMethods[] = {
				{"set", "void set(const string &in key, const ?&in value)", "Stores value in the dictionary under key."},
				{"get", "bool get(const string &in key, ?&out value) const", "Retrieves the value stored under key into value. Returns false if key does not exist."},
				{"exists", "bool exists(const string &in key) const", "Returns true if key is present in the dictionary."},
				{"delete", "bool delete(const string &in key)", "Removes the entry for key. Returns true if the key was present."},
				{"deleteAll", "void deleteAll()", "Removes all entries from the dictionary."},
				{"getSize", "uint getSize() const", "Returns the number of key/value pairs in the dictionary."},
				{"getKeys", "array<string>@ getKeys() const", "Returns an array containing all keys currently stored in the dictionary."},
			};
			//
			// grid<T> addon methods (scriptgrid)
			//
			static const ApiSymbolDef kApiGridMethods[] = {
				{"getWidth", "uint getWidth() const", "Returns the number of columns in the grid."},
				{"getHeight", "uint getHeight() const", "Returns the number of rows in the grid."},
				{"resize", "void resize(uint width, uint height)", "Resizes the grid to width columns and height rows."},
			};
			//
			// Math global functions (scriptmath)
			//
			static const ApiSymbolDef kApiMathFunctions[] = {
				{"cos", "float cos(float rad)", "Returns the cosine of rad (radians)."},
				{"sin", "float sin(float rad)", "Returns the sine of rad (radians)."},
				{"tan", "float tan(float rad)", "Returns the tangent of rad (radians)."},
				{"acos", "float acos(float x)", "Returns the arc-cosine of x in radians."},
				{"asin", "float asin(float x)", "Returns the arc-sine of x in radians."},
				{"atan", "float atan(float x)", "Returns the arc-tangent of x in radians."},
				{"atan2", "float atan2(float y, float x)", "Returns the arc-tangent of y/x, using signs to determine the quadrant."},
				{"cosh", "float cosh(float x)", "Returns the hyperbolic cosine of x."},
				{"sinh", "float sinh(float x)", "Returns the hyperbolic sine of x."},
				{"tanh", "float tanh(float x)", "Returns the hyperbolic tangent of x."},
				{"log", "float log(float x)", "Returns the natural logarithm of x."},
				{"log10", "float log10(float x)", "Returns the base-10 logarithm of x."},
				{"pow", "float pow(float base, float exp)", "Returns base raised to the power exp."},
				{"sqrt", "float sqrt(float x)", "Returns the square root of x."},
				{"abs", "float abs(float x)", "Returns the absolute value of x."},
				{"ceil", "float ceil(float x)", "Returns the smallest integer not less than x."},
				{"floor", "float floor(float x)", "Returns the largest integer not greater than x."},
				{"fraction", "float fraction(float x)", "Returns the fractional part of x."},
			};
			//
			// string addon methods (scriptstdstring)
			//
			static const ApiSymbolDef kApiStringMethods[] = {
				{"length", "uint length() const", "Returns the number of characters in the string."},
				{"resize", "void resize(uint newLength)", "Resizes the string to newLength characters."},
				{"isEmpty", "bool isEmpty() const", "Returns true if the string contains no characters."},
				{"substr", "string substr(uint start = 0, int count = -1) const", "Returns a substring starting at start with up to count characters."},
				{"findFirst", "int findFirst(const string &in sub, uint start = 0) const", "Returns the index of the first occurrence of sub at or after start, or -1."},
				{"findFirstOf", "int findFirstOf(const string &in chars, uint start = 0) const",
				 "Returns the index of the first character in chars found at or after start, or -1."},
				{"findFirstNotOf", "int findFirstNotOf(const string &in chars, uint start = 0) const",
				 "Returns the index of the first character not in chars at or after start, or -1."},
				{"findLast", "int findLast(const string &in sub, int start = -1) const", "Returns the index of the last occurrence of sub, searching backward from start."},
				{"findLastOf", "int findLastOf(const string &in chars, int start = -1) const", "Returns the index of the last character in chars, searching backward from start."},
				{"findLastNotOf", "int findLastNotOf(const string &in chars, int start = -1) const",
				 "Returns the index of the last character not in chars, searching backward from start."},
				{"insert", "void insert(uint pos, const string &in other)", "Inserts other into the string at position pos."},
				{"erase", "void erase(uint pos, int count = -1)", "Removes count characters from the string starting at pos."},
				{"regexFind", "int regexFind(const string &in regex, uint start = 0, uint &out lengthOfMatch = void) const",
				 "Finds the first match of regex at or after start. Stores match length in lengthOfMatch."},
			};
			//
			// String global utility functions (scriptstdstring)
			//
			static const ApiSymbolDef kApiStringGlobalFunctions[] = {
				{"format", "string format(const string &in fmt, const ?&in ...)", "Returns a formatted string using fmt as a format specifier."},
				{"formatInt", "string formatInt(int64 val, const string &in options = \"\", uint width = 0)",
				 "Formats an integer value as a string with optional options and minimum width."},
				{"formatUInt", "string formatUInt(uint64 val, const string &in options = \"\", uint width = 0)",
				 "Formats an unsigned integer value as a string with optional options and minimum width."},
				{"formatFloat", "string formatFloat(double val, const string &in options = \"\", uint width = 0, uint precision = 0)",
				 "Formats a floating-point value as a string."},
				{"parseInt", "int64 parseInt(const string &in str, uint base = 10, uint &out byteCount = 0)",
				 "Parses an integer from str in the given base. Stores the number of bytes consumed."},
				{"parseUInt", "uint64 parseUInt(const string &in str, uint base = 10, uint &out byteCount = 0)", "Parses an unsigned integer from str in the given base."},
				{"parseFloat", "double parseFloat(const string &in str, uint &out byteCount = 0)", "Parses a floating-point number from str. Stores the number of bytes consumed."},
			};
			//
			// file object methods (scriptfile)
			//
			static const ApiSymbolDef kApiFileMethods[] = {
				{"open", "int open(const string &in filename, const string &in mode)",
				 "Opens a file. mode: \"r\" read, \"w\" write (overwrites), \"a\" append. Returns 0 on success."},
				{"close", "int close()", "Closes the file. Returns 0 on success."},
				{"getSize", "int getSize() const", "Returns the size of the file in bytes."},
				{"isEOF", "bool isEOF() const", "Returns true if the file cursor is at the end of the file."},
				{"readString", "string readString(uint length)", "Reads up to length bytes from the file and returns them as a string."},
				{"readLine", "string readLine()", "Reads one line from the file, including the newline character."},
				{"readInt", "int64 readInt(uint bytes)", "Reads bytes bytes as a signed integer."},
				{"readUInt", "uint64 readUInt(uint bytes)", "Reads bytes bytes as an unsigned integer."},
				{"readFloat", "float readFloat()", "Reads a 4-byte float from the file."},
				{"readDouble", "double readDouble()", "Reads an 8-byte double from the file."},
				{"writeString", "int writeString(const string &in str)", "Writes str to the file. Returns 0 on success."},
				{"writeInt", "int writeInt(int64 value, uint bytes)", "Writes value as a bytes-wide signed integer."},
				{"writeUInt", "int writeUInt(uint64 value, uint bytes)", "Writes value as a bytes-wide unsigned integer."},
				{"writeFloat", "int writeFloat(float value)", "Writes a 4-byte float to the file."},
				{"writeDouble", "int writeDouble(double value)", "Writes an 8-byte double to the file."},
				{"getPos", "int getPos() const", "Returns the current file cursor position."},
				{"setPos", "int setPos(int pos)", "Sets the file cursor to an absolute position. Returns 0 on success."},
				{"movePos", "int movePos(int delta)", "Moves the file cursor by delta bytes relative to its current position."},
			};
			//
			// filesystem object methods (scriptfilesystem)
			//
			static const ApiSymbolDef kApiFileSystemMethods[] = {
				{"changeCurrentPath", "bool changeCurrentPath(const string &in path)",
				 "Changes the current working path. Supports relative paths (use \"..\" to go up). Returns true on success."},
				{"getCurrentPath", "string getCurrentPath() const", "Returns the current working path."},
				{"isDir", "bool isDir(const string &in path) const", "Returns true if path points to an existing directory."},
				{"isLink", "bool isLink(const string &in path) const", "Returns true if path is a symbolic link."},
				{"getSize", "int64 getSize(const string &in path) const", "Returns the size in bytes of the file at path."},
				{"getFiles", "array<string>@ getFiles() const", "Returns an array of file names in the current path."},
				{"getDirs", "array<string>@ getDirs() const", "Returns an array of subdirectory names in the current path."},
				{"makeDir", "int makeDir(const string &in path)", "Creates a new directory at path. Returns 0 on success."},
				{"removeDir", "int removeDir(const string &in path)", "Removes an empty directory at path. Returns 0 on success."},
				{"deleteFile", "int deleteFile(const string &in path)", "Deletes the file at path. Returns 0 on success."},
				{"copyFile", "int copyFile(const string &in source, const string &in target)", "Copies source to target. Returns 0 on success."},
				{"move", "int move(const string &in source, const string &in target)", "Moves or renames source to target. Returns 0 on success."},
			};
			//
			// Resolve an #include path relative to the including file's directory.
			//
			static std::string ResolveIncludePath(const std::string& fromFile, const std::string& includePath) {
				std::filesystem::path base = std::filesystem::path(fromFile).parent_path();
				std::filesystem::path resolved = base / includePath;
				std::error_code ec;
				std::filesystem::path canonical = std::filesystem::weakly_canonical(resolved, ec);
				if (ec)
					return resolved.string();
				return canonical.string();
			}
			//
			// AngelScriptParser — per-parse-job stateful parser.
			//
			class AngelScriptParser : public ILanguageParser {
			public:
				AngelScriptParser(TextEditorMI* host, const ILanguageDefinition* langDef)
					: mHost(host), mLangDef(langDef) {}

				void ParseStart(const std::string& filePath) override {
					mCurrentFilePath = filePath;
					mRecentLines.clear();
					mBraceTracker = mHost->AllocateBlockTracker();
					mBraceBlockIds.clear();
					InjectApiSymbols();
				}

				void ParseLine(int lineNumber, const std::string& filePath, const std::string& lineText) override {
					mRecentLines.push_back(std::make_pair(lineNumber, lineText));
					if (mRecentLines.size() > 30)
						mRecentLines.pop_front();
					std::string_view sv(lineText);
					//
					// Skip leading whitespace
					//
					size_t p = 0;
					while (p < sv.size() && (sv[p] == ' ' || sv[p] == '\t'))
						p++;
					if (p >= sv.size())
						return;
					//
					// Skip comment lines — they cannot start a symbol definition
					//
					if (p + 1 < sv.size() && sv[p] == '/' && sv[p + 1] == '/')
						return;
					//
					// #include "path/to/file.as"
					//
					if (sv.substr(p, 8) == "#include") {
						size_t q = p + 8;
						while (q < sv.size() && (sv[q] == ' ' || sv[q] == '\t'))
							q++;
						if (q < sv.size() && sv[q] == '"') {
							size_t start = q + 1;
							size_t end = sv.find('"', start);
							if (end != std::string_view::npos) {
								std::string includeName(sv.substr(start, end - start));
								std::string includePath = ResolveIncludePath(filePath, includeName);
								mHost->EnqueueCodeLensFile(includePath, mLangDef);
							}
						}
						return;
					}
					//
					// Function definition detection.
					// Pattern: at least one word (return type) then FuncName(
					// The CTRE search finds the last identifier immediately before '('.
					//
					std::string_view rest = sv.substr(p);
					if (auto m = ctre::search<R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\()">(rest)) {
						//
						// Skip flow-control keywords that cannot begin a declaration
						//
						static const char* kSkipKeywords[] = {"if",	   "else",	   "for",	  "while", "do",	 "switch", "case", "return",
															  "break", "continue", "default", "new",   "delete", "null",   "true", "false"};
						std::string funcName(m.get<1>().to_view());
						bool skip = false;
						for (size_t ki = 0; ki < sizeof(kSkipKeywords) / sizeof(kSkipKeywords[0]); ki++)
							if (funcName == kSkipKeywords[ki]) {
								skip = true;
								break;
							}
						if (skip)
							return;
						//
						// Require that something (the return type) precedes the function name
						//
						ptrdiff_t matchOffset = m.get<0>().data() - rest.data();
						if (matchOffset <= 0)
							return;
						//
						// Require a closing ')' or opening '{' on the same line, or an unclosed
						// '(' with no ';' — the last case covers multi-line function signatures.
						//
						std::string_view afterName = rest.substr(static_cast<size_t>(matchOffset) + funcName.size());
						bool hasBrace = afterName.find('{') != std::string_view::npos;
						bool hasCloseParen = afterName.find(')') != std::string_view::npos;
						bool hasOpenParen = afterName.find('(') != std::string_view::npos || rest.substr(0, static_cast<size_t>(matchOffset) + funcName.size()).find('(') != std::string_view::npos;
						bool hasSemicolon = rest.find(';') != std::string_view::npos;
						bool isMultiLineSig = hasOpenParen && !hasCloseParen && !hasSemicolon;
						if (!hasCloseParen && !hasBrace && !isMultiLineSig)
							return;
						//
						// Skip names already covered by the static API definitions — they are call
						// sites, not user function definitions, and must not shadow the API docs.
						//
						if (IsApiSymbol(funcName))
							return;
						//
						// Build the symbol with any leading // doc comment as documentation
						//
						std::string comment = mHost->ExtractLeadingComment(mRecentLines, lineNumber, "//");
						std::string externalCode = lineText;
						if (!comment.empty())
							externalCode = comment + "\n\n" + lineText;
						TextEditor::CodeLensSymbolData sym;
						sym.symbolName = funcName;
						sym.lineNumber = lineNumber;
						sym.codelensText = "";
						sym.externalCode = externalCode;
						sym.kind = TextEditor::CodeLensSymbolKind::UserDefined;
						mHost->AddCodeLensSymbol(filePath, sym);
					}
					//
					// Brace tracking — scan for { and } while skipping strings and // comments.
					// Pattern matches: // (stop), "..." (skip), { or } (track)
					//
					{
						std::string_view scanView(lineText);
						while (!scanView.empty()) {
							auto bm = ctre::search<R"(//|"(?:[^"\\]|\\.)*"|[{}])">(scanView);
							if (!bm)
								break;
							std::string_view matched = bm.get<0>().to_view();
							size_t offset = static_cast<size_t>(matched.data() - scanView.data());
							char first = matched[0];
							if (first == '/')
								break;
							if (first == '"') {
								scanView = scanView.substr(offset + matched.size());
								continue;
							}
							int col = static_cast<int>(matched.data() - lineText.data());
							if (first == '{') {
								int id = mHost->OpenBlock(mBraceTracker, lineNumber, col, "opening brace");
								mBraceBlockIds.push_back(id);
							} else {
								if (!mBraceBlockIds.empty()) {
									mHost->CloseBlock(mBraceTracker, mBraceBlockIds.back());
									mBraceBlockIds.pop_back();
								}
							}
							scanView = scanView.substr(offset + 1);
						}
					}
				}

				void ParseEnd(const std::string& filePath, const std::vector<UnclosedBlock>& unclosedBlocks) override {
					for (size_t i = 0; i < unclosedBlocks.size(); i++)
						mHost->AddCodeLensError(filePath, unclosedBlocks[i].line,
							"Unmatched '{' at line " + std::to_string(unclosedBlocks[i].line));
					mRecentLines.clear();
					mCurrentFilePath.clear();
					mBraceBlockIds.clear();
				}

				bool SyntaxHighlight(const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd,
				                     TextEditor::PaletteIndex& paletteIndex) const override {
					paletteIndex = TextEditor::PaletteIndex::Max;
					while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
						inBegin++;
					if (inBegin == inEnd) {
						outBegin = inEnd;
						outEnd = inEnd;
						paletteIndex = TextEditor::PaletteIndex::Default;
						return true;
					}
					std::string_view sv(inBegin, static_cast<size_t>(inEnd - inBegin));
					//
					// Triple-quoted multiline string  """..."""
					//
					if (auto m = ctre::starts_with<R"("""(?:[^"]|"(?!"")|""(?!"))*""")">(sv)) {
						outBegin = inBegin;
						outEnd = inBegin + m.size();
						paletteIndex = TextEditor::PaletteIndex::String;
						return true;
					}
					//
					// Double-quoted string  "..."  with escape sequences
					//
					if (auto m = ctre::starts_with<R"("(?:[^"\\]|\\.)*")">(sv)) {
						outBegin = inBegin;
						outEnd = inBegin + m.size();
						paletteIndex = TextEditor::PaletteIndex::String;
						return true;
					}
					//
					// Char literal  '.'  (single char or one escape sequence)
					//
					if (auto m = ctre::starts_with<R"('(?:\\.|[^'\\])')">(sv)) {
						outBegin = inBegin;
						outEnd = inBegin + m.size();
						paletteIndex = TextEditor::PaletteIndex::CharLiteral;
						return true;
					}
					//
					// @handle reference  @identifier  -> Preprocessor colour
					//
					if (auto m = ctre::starts_with<R"(@[a-zA-Z_][a-zA-Z0-9_]*)">(sv)) {
						outBegin = inBegin;
						outEnd = inBegin + m.size();
						paletteIndex = TextEditor::PaletteIndex::Preprocessor;
						return true;
					}
					//
					// Identifier  [a-zA-Z_][a-zA-Z0-9_]*
					//
					if (auto m = ctre::starts_with<R"([a-zA-Z_][a-zA-Z0-9_]*)">(sv)) {
						outBegin = inBegin;
						outEnd = inBegin + m.size();
						paletteIndex = TextEditor::PaletteIndex::Identifier;
						return true;
					}
					//
					// Number: binary 0b, hex 0x, float with optional exponent, plain integer
					//
					if (auto m = ctre::starts_with<R"(0[bB][01]+|0[xX][0-9a-fA-F]+|[0-9]+([.][0-9]*)?([eE]([+]|\x2D)?[0-9]+)?f?)">(sv)) {
						outBegin = inBegin;
						outEnd = inBegin + m.size();
						paletteIndex = TextEditor::PaletteIndex::Number;
						return true;
					}
					//
					// Punctuation: single-character operators and delimiters
					//
					switch (*inBegin) {
						case '[':
						case ']':
						case '{':
						case '}':
						case '!':
						case '%':
						case '^':
						case '&':
						case '*':
						case '(':
						case ')':
						case '-':
						case '+':
						case '=':
						case '~':
						case '|':
						case '<':
						case '>':
						case '?':
						case ':':
						case '/':
						case ';':
						case ',':
						case '.':
						case '@':
							outBegin = inBegin;
							outEnd = inBegin + 1;
							paletteIndex = TextEditor::PaletteIndex::Punctuation;
							return true;
					}
					return false;
				}

			private:
				TextEditorMI* mHost;
				const ILanguageDefinition* mLangDef;
				std::string mCurrentFilePath;
				std::deque<std::pair<int, std::string>> mRecentLines;
				int mBraceTracker = -1;
				std::vector<int> mBraceBlockIds;

				bool HasApiSymbolsInjected() const {
					const auto& files = mHost->GetCodeLensFiles();
					for (size_t fi = 0; fi < files.size(); fi++)
						if (files[fi].filePath == "<angelscript-api>" && !files[fi].symbols.empty())
							return true;
					return false;
				}

				bool IsApiSymbol(const std::string& name) const {
					const auto& files = mHost->GetCodeLensFiles();
					for (size_t fi = 0; fi < files.size(); fi++) {
						if (files[fi].filePath != "<angelscript-api>")
							continue;
						for (size_t si = 0; si < files[fi].symbols.size(); si++)
							if (files[fi].symbols[si].symbolName == name)
								return true;
					}
					return false;
				}

				void InjectApiSymbols() {
					if (HasApiSymbolsInjected())
						return;
					const std::string apiFile = "<angelscript-api>";
					auto addSym = [&](const ApiSymbolDef& def) {
						TextEditor::CodeLensSymbolData sym;
						sym.symbolName = def.name;
						sym.lineNumber = -1;
						sym.codelensText = "";
						sym.externalCode = std::string(def.signature) + "\n\n" + def.description;
						sym.kind = TextEditor::CodeLensSymbolKind::BuiltIn;
						mHost->AddCodeLensSymbolIfNew(apiFile, sym);
					};
					for (size_t i = 0; i < sizeof(kApiGlobalFunctions) / sizeof(kApiGlobalFunctions[0]); i++)
						addSym(kApiGlobalFunctions[i]);
					for (size_t i = 0; i < sizeof(kApiImageMethods) / sizeof(kApiImageMethods[0]); i++)
						addSym(kApiImageMethods[i]);
					for (size_t i = 0; i < sizeof(kApiPaletteMethods) / sizeof(kApiPaletteMethods[0]); i++)
						addSym(kApiPaletteMethods[i]);
					for (size_t i = 0; i < sizeof(kApiBitmapContextMethods) / sizeof(kApiBitmapContextMethods[0]); i++)
						addSym(kApiBitmapContextMethods[i]);
					for (size_t i = 0; i < sizeof(kApiTilesetContextMethods) / sizeof(kApiTilesetContextMethods[0]); i++)
						addSym(kApiTilesetContextMethods[i]);
					for (size_t i = 0; i < sizeof(kApiSpriteContextMethods) / sizeof(kApiSpriteContextMethods[0]); i++)
						addSym(kApiSpriteContextMethods[i]);
					for (size_t i = 0; i < sizeof(kApiMapContextMethods) / sizeof(kApiMapContextMethods[0]); i++)
						addSym(kApiMapContextMethods[i]);
					for (size_t i = 0; i < sizeof(kApiRgbColorFields) / sizeof(kApiRgbColorFields[0]); i++)
						addSym(kApiRgbColorFields[i]);
					for (size_t i = 0; i < sizeof(kApiArrayMethods) / sizeof(kApiArrayMethods[0]); i++)
						addSym(kApiArrayMethods[i]);
					for (size_t i = 0; i < sizeof(kApiDictionaryMethods) / sizeof(kApiDictionaryMethods[0]); i++)
						addSym(kApiDictionaryMethods[i]);
					for (size_t i = 0; i < sizeof(kApiGridMethods) / sizeof(kApiGridMethods[0]); i++)
						addSym(kApiGridMethods[i]);
					for (size_t i = 0; i < sizeof(kApiMathFunctions) / sizeof(kApiMathFunctions[0]); i++)
						addSym(kApiMathFunctions[i]);
					for (size_t i = 0; i < sizeof(kApiStringMethods) / sizeof(kApiStringMethods[0]); i++)
						addSym(kApiStringMethods[i]);
					for (size_t i = 0; i < sizeof(kApiStringGlobalFunctions) / sizeof(kApiStringGlobalFunctions[0]); i++)
						addSym(kApiStringGlobalFunctions[i]);
					for (size_t i = 0; i < sizeof(kApiFileMethods) / sizeof(kApiFileMethods[0]); i++)
						addSym(kApiFileMethods[i]);
					for (size_t i = 0; i < sizeof(kApiFileSystemMethods) / sizeof(kApiFileSystemMethods[0]); i++)
						addSym(kApiFileSystemMethods[i]);
				}
			};
		}
	}
}

namespace RetrodevGui {

	AngelScriptLanguage::AngelScriptLanguage() {
		for (auto& k : ImGui::TextEditorLangs::AngelScript::kAngelScriptKeywords)
			mKeywords.insert(k);
		for (auto& k : ImGui::TextEditorLangs::AngelScript::kAngelScriptIdentifiers) {
			ImGui::LangIdentifier id;
			id.mDeclaration = "Built-in function";
			mIdentifiers.insert(std::make_pair(std::string(k), id));
		}
		mCommentStart = "/*";
		mCommentEnd = "*/";
		mSingleLineComment = "//";
		mCaseSensitive = true;
		mName = "AngelScript";
	}

	ImGui::ILanguageParser* AngelScriptLanguage::CreateParser(ImGui::TextEditorMI* host) const {
		return new ImGui::TextEditorLangs::AngelScript::AngelScriptParser(host, this);
	}

}
