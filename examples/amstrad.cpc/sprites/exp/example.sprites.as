// ---------------------------------------------------------------------------
//
// Retrodev Examples
//
// example.sprites.as -- exports CPC sprites as raw byte data.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------
//
// Script metadata
//
// @description Exports CPC sprites as raw byte data, one sprite after another.
// @exporter sprites
// @target amstrad.cpc
// @param exportPalette  bool   false                                   Export palette (.pal file)
// @param paletteFormat  combo  systemindex  systemindex|hardwareindex|hardwarecmd  Palette format
//
// ---------------------------------------------------------------------------
//
// Each sprite is exported as a contiguous block of bytes.
// Sprites can have arbitrary sizes; each row is encoded according to the
// active CPC video mode pixel-per-byte count:
//
//   Mode 0 -- 2 pixels per byte, 16 colours
//   Mode 1 -- 4 pixels per byte,  4 colours
//   Mode 2 -- 8 pixels per byte,  2 colours
//
// Rows are emitted top-to-bottom.  Sprites with a width that is not a
// multiple of the mode's pixels-per-byte count are rejected with an error.
//
// Output is an assembler text file (.asm).
// Each sprite is written as:
//   <sanitized_label>:
//     db <width_in_bytes>, <height>
//     db <encoded bytes...>
//
// Sprite count header is not emitted.
//
// ---------------------------------------------------------------------------

#include "cpc.utils.as"

string HexByte(uint8 value)
{
    const string hex = "0123456789ABCDEF";
  string text = "0x";
    text += hex.substr((value >> 4) & 0x0F, 1);
    text += hex.substr(value & 0x0F, 1);
    return text;
}

void WriteText(file@ f, const string& in text)
{
    f.writeString(text);
}

string SanitizeLabel(const string& in name, int index)
{
    string label = "";
    bool lastUnderscore = false;
    for (uint i = 0; i < name.length(); i++) {
     string ch = name.substr(i, 1);
        bool isLetter = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
        bool isDigit = (ch >= '0' && ch <= '9');
        if (isLetter || isDigit || ch == '_') {
           label += name.substr(i, 1);
            lastUnderscore = false;
        } else {
            if (!lastUnderscore) {
             label += "_";
                lastUnderscore = true;
            }
        }
    }
    if (label.length() == 0)
        label = "sprite";
    string first = label.substr(0, 1);
    if (first >= '0' && first <= '9')
        label = "_" + label;
    return "spr_" + label + "_" + index;
}

string ChangeExtension(const string& in path, const string& in extWithDot)
{
    int dotPos = path.findLast(".");
    if (dotPos >= 0)
        return path.substr(0, dotPos) + extWithDot;
    return path + extWithDot;
}

// ---------------------------------------------------------------------------
//
// Export entry point.
//
// void Export(const string& in outputPath, SpriteExportContext@ ctx)
//
// ---------------------------------------------------------------------------

void Export(const string& in outputPath, SpriteExportContext@ ctx)
{
    int spriteCount = ctx.GetSpriteCount();
    string mode = ctx.GetTargetMode();
    string system = ctx.GetTargetSystem();
    Palette@ palette = ctx.GetPalette();
    bool exportPalette = ctx.GetParam("exportPalette") == "true";
    string paletteFormat = ctx.GetParam("paletteFormat");
    if (spriteCount <= 0) {
        Log_Error("Export failed: no sprites extracted -- run extraction first.");
        return;
    }
    int ppb = 2;
    if (mode == "Mode 1") ppb = 4;
    else if (mode == "Mode 2") ppb = 8;
    string asmPath = ChangeExtension(outputPath, ".asm");
    Log_Info("Export started -- system: " + system + "  mode: " + mode
        + "  sprites: " + spriteCount
        + "  output: " + asmPath);
    for (int i = 0; i < spriteCount; i++) {
        int w = ctx.GetSpriteWidth(i);
        if ((w % ppb) != 0) {
            Log_Error("Export failed: sprite #" + i + " (" + ctx.GetSpriteName(i)
                + ") width " + w + " is not a multiple of " + ppb
                + " for " + mode + ".");
            return;
        }
    }
    file@ f = file();
    if (f.open(asmPath, "w") < 0) {
        Log_Error("Export failed: cannot open output file: " + asmPath);
        return;
    }
    WriteText(f, "; Sprite data exported by example.sprites.as\n");
    WriteText(f, "; system: " + system + "  mode: " + mode + "\n\n");
    for (int i = 0; i < spriteCount; i++) {
        Image@ img = ctx.GetSprite(i);
        if (img is null) {
            f.close();
            Log_Error("Export failed: GetSprite(" + i + ") returned null.");
            return;
        }
        int w = ctx.GetSpriteWidth(i);
        int h = ctx.GetSpriteHeight(i);
        int bytesPerRow = w / ppb;
        array<uint8> spriteBuf;
        spriteBuf.insertLast(uint8(bytesPerRow));
        spriteBuf.insertLast(uint8(h));
        array<int> penRow(w);
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                RgbColor px = img.GetPixelColor(col, row);
                int pen = palette.PenGetIndex(px);
                if (pen < 0) {
                    Log_Warning("Sprite #" + i + " pixel " + col + "," + row
                        + " rgb(" + px.r + "," + px.g + "," + px.b + ")"
                        + " has no matching pen -- using pen 0.");
                    pen = 0;
                }
                penRow[col] = pen;
            }
            EncodePixels(penRow, 0, w, mode, spriteBuf);
        }
        string label = SanitizeLabel(ctx.GetSpriteName(i), i);
        WriteText(f, label + ":\n");
        for (uint p = 0; p < spriteBuf.length(); p++) {
            if ((p % 16) == 0)
                WriteText(f, "\tdb ");
            WriteText(f, HexByte(spriteBuf[p]));
            if ((p % 16) == 15 || p == spriteBuf.length() - 1)
                WriteText(f, "\n");
            else
                WriteText(f, ", ");
        }
        WriteText(f, "\n");
        Log_Info("Sprite #" + i + " '" + ctx.GetSpriteName(i)
            + "' -> " + label + "  " + w + "x" + h
            + " -> " + (h * bytesPerRow + 2) + " bytes");
    }
    f.close();
    Log_Info("Export complete -- asm written to: " + asmPath);
    if (exportPalette) {
        array<uint8> palBuf;
        int penCount = palette.PaletteMaxColors();
        for (int pen = 0; pen < penCount; pen++) {
            int sysIdx = palette.PenGetColorIndex(pen);
            uint8 palByte;
            if (paletteFormat == "hardwareindex") {
                int hwIdx = CpcHardwareColorIndex(sysIdx);
                palByte = uint8(hwIdx >= 0 ? hwIdx : 0);
            } else if (paletteFormat == "hardwarecmd") {
                int cmdIdx = CpcHardwareCmdColorIndex(sysIdx);
                palByte = uint8(cmdIdx >= 0 ? cmdIdx : 0);
            } else {
                palByte = uint8(sysIdx >= 0 ? sysIdx : 0);
            }
            palBuf.insertLast(palByte);
        }
        string palPath = ChangeExtension(outputPath, ".pal");
        file@ pf = file();
        if (pf.open(palPath, "w") < 0) {
            Log_Error("Palette export failed: could not open output file: " + palPath);
        } else {
            for (uint i = 0; i < palBuf.length(); i++)
                pf.writeUInt(palBuf[i], 1);
            pf.close();
            Log_Info("Palette exported -- " + palBuf.length() + " bytes written to: " + palPath);
        }
    }
}
