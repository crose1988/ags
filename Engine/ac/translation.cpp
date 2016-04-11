//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#define USE_CLIB
#include <stdio.h>
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/runtime_defines.h"
#include "ac/translation.h"
#include "ac/tree_map.h"
#include "debug/out.h"
#include "util/misc.h"
#include "util/stream.h"
#include "util/path.h"
#include "core/assetmanager.h"

using AGS::Common::Stream;
using AGS::Common::String;
namespace Out = AGS::Common::Out;

extern GameSetup usetup;
extern GameSetupStruct game;
extern GameState play;
extern char transFileName[MAX_PATH];
extern char *untransFileLoc;

TreeMap *transtree = NULL;
TreeMap *untranstree = NULL;
long lang_offs_start = 0;
char transFileName[MAX_PATH] = "\0";
char *untransFileLoc = "\0";

void close_translation () {
    if (transtree != NULL) {
        delete transtree;
        transtree = NULL;
    }
}

bool parse_translation(Stream *language_file, String &parse_error);

bool parse_chinaavg_translation(const char *language_file, String &parse_error);

bool parse_chinaavg_untranslation(const char *language_file, String &parse_error);

bool init_translation (const String &lang, const String &fallback_lang, bool quit_on_error) {
    char *transFileLoc;

    if (lang.IsEmpty()) {
        sprintf(transFileName, "default.tra");
    }
    else {
        sprintf(transFileName, "%s.tra", lang.GetCStr());
    }

    transFileLoc = ci_find_file(usetup.data_files_dir, transFileName);

    Stream *language_file = Common::AssetManager::OpenAsset(transFileLoc);
    free(transFileLoc);

    if (language_file == NULL) 
    {
        Out::FPrint("Cannot open translation: %s", transFileName);
        if (!lang.IsEmpty())
        {
            // Just in case they're running in Debug, try compiled folder
            sprintf(transFileName, "Compiled\\%s.tra", lang.GetCStr());
            language_file = Common::AssetManager::OpenAsset(transFileName);
            if (!language_file)
                Out::FPrint("Cannot open translation: %s", transFileName);
        }
        if (language_file == NULL)
            return false;
    }
    // in case it's inside a library file, record the offset
    lang_offs_start = language_file->GetPosition();

    char transsig[16] = {0};
    language_file->Read(transsig, 15);
    if (strcmp(transsig, "AGSTranslation") != 0) {
        Out::FPrint("Translation signature mismatch: %s", transFileName);
        delete language_file;
        return false;
    }

    if (transtree != NULL)
    {
        close_translation();
    }
    transtree = new TreeMap();

    String parse_error;
    bool result = parse_translation(language_file, parse_error);
    delete language_file;

    if (!result)
    {
        close_translation();
        parse_error.Prepend(String::FromFormat("Failed to read translation file: %s:\n", transFileName));
        if (quit_on_error)
        {
            parse_error.PrependChar('!');
            quit(parse_error);
        }
        else
        {
            Out::FPrint(parse_error);
            if (!fallback_lang.IsEmpty())
            {
                Out::FPrint("Fallback to translation: %s", fallback_lang.GetCStr());
                init_translation(fallback_lang, "", false);
            }
            return false;
        }
    }
    Out::FPrint("Translation initialized: %s", transFileName);
    return true;
}

bool init_chinaavg_translation(const String &lang, const String &fallback_lang, bool quit_on_error) {
    char *transFileLoc;

    if (lang.IsEmpty()) {
        sprintf(transFileName, "default.trs");
    }
    else {
        sprintf(transFileName, "%s.trs", lang.GetCStr());
    }

    transFileLoc = ci_find_file(usetup.chinaavg_dir, transFileName);

    if (!AGS::Common::Path::IsFile(transFileLoc)) {
        Out::FPrint("Cannot open ChinaAVG translation: %s", transFileName);
        free(transFileLoc);
        return false;
    }

    if (transtree != NULL)
    {
        close_translation();
    }
    transtree = new TreeMap();

    String parse_error;
    bool result = parse_chinaavg_translation(AGS::Common::Path::MakeAbsolutePath(transFileLoc).GetCStr(), parse_error);
    free(transFileLoc);

    if (!result)
    {
        close_translation();
        parse_error.Prepend(String::FromFormat("Failed to read ChinaAVG translation file: %s:\n", transFileName));
        if (quit_on_error)
        {
            parse_error.PrependChar('!');
            quit(parse_error);
        }
        else
        {
            Out::FPrint(parse_error);
            if (!fallback_lang.IsEmpty())
            {
                Out::FPrint("Fallback to ChinaAVG translation: %s", fallback_lang.GetCStr());
                init_chinaavg_translation(fallback_lang, "", false);
            }
            return false;
        }
    }
    Out::FPrint("ChinaAVG translation initialized: %s", transFileName);
    return true;
}

bool init_chinaavg_untranslation(const String &lang) {
    char untransFileName[MAX_PATH];

    if (lang.IsEmpty()) {
        sprintf(untransFileName, "default_untrans.trs");
    }
    else {
        sprintf(untransFileName, "%s_untrans.trs", lang.GetCStr());
    }

    untransFileLoc = ci_find_file(usetup.chinaavg_dir, untransFileName);

    if (!AGS::Common::Path::IsFile(untransFileLoc)) {
        Out::FPrint("Cannot open ChinaAVG untranslation: %s", untransFileName);
        return false;
    }

    if (untranstree != NULL) {
        delete untranstree;
        untranstree = NULL;
    }
    untranstree = new TreeMap();
    String parse_error;
    bool result = parse_chinaavg_untranslation(AGS::Common::Path::MakeAbsolutePath(untransFileLoc).GetCStr(), parse_error);

    if (!result)
    {
        if (untranstree != NULL) {
            delete untranstree;
            untranstree = NULL;
        }
        parse_error.Prepend(String::FromFormat("Failed to read ChinaAVG untranslation file: %s:\n", untransFileName));
        Out::FPrint(parse_error);
        return false;
    }

    Out::FPrint("ChinaAVG untranslation initialized: %s", untransFileName);
    return true;

}

bool parse_translation(Stream *language_file, String &parse_error)
{
    while (!language_file->EOS()) {
        int blockType = language_file->ReadInt32();
        if (blockType == -1)
            break;
        // MACPORT FIX 9/6/5: remove warning
        /* int blockSize = */ language_file->ReadInt32();

        if (blockType == 1) {
            char original[STD_BUFFER_SIZE], translation[STD_BUFFER_SIZE];
            while (1) {
                read_string_decrypt (language_file, original);
                read_string_decrypt (language_file, translation);
                if ((strlen (original) < 1) && (strlen(translation) < 1))
                    break;
                if (language_file->EOS())
                {
                    parse_error = "Translation file is corrupt";
                    return false;
                }
                transtree->addText (original, translation);
            }

        }
        else if (blockType == 2) {
            int uidfrom;
            char wasgamename[100];
            uidfrom = language_file->ReadInt32();
            read_string_decrypt (language_file, wasgamename);
            if ((uidfrom != game.uniqueid) || (strcmp (wasgamename, game.gamename) != 0)) {
                parse_error.Format("The translation file is not compatible with this game. The translation is designed for '%s'.",
                    wasgamename);
                return false;
            }
        }
        else if (blockType == 3) {
            // game settings
            int temp = language_file->ReadInt32();
            // normal font
            if (temp >= 0)
                SetNormalFont (temp);
            temp = language_file->ReadInt32();
            // speech font
            if (temp >= 0)
                SetSpeechFont (temp);
            temp = language_file->ReadInt32();
            // text direction
            if (temp == 1) {
                play.text_align = SCALIGN_LEFT;
                game.options[OPT_RIGHTLEFTWRITE] = 0;
            }
            else if (temp == 2) {
                play.text_align = SCALIGN_RIGHT;
                game.options[OPT_RIGHTLEFTWRITE] = 1;
            }
        }
        else
        {
            parse_error.Format("Unknown block type in translation file (%d).", blockType);
            return false;
        }
    }

    if (transtree->text == NULL)
    {
        parse_error = "The translation file was empty.";
        return false;
    }

    return true;
}

bool parse_chinaavg_translation(const char *language_file, String &parse_error) {
    bool isTrans = false;
    char line[STD_BUFFER_SIZE];
    String original, translation;
    FILE *fp;   
    if ((fp = fopen(language_file,"r")) == NULL) {
        parse_error = "Can't open ChinaAVG translation file.";
        return false;
    }

    while(!feof(fp)) {
        fgets(line, STD_BUFFER_SIZE, fp);
        if (!isTrans) {
            original = line;
            isTrans = true;
            original.Trim();
        } 
        else {
            translation = line;
            isTrans = false;
            translation.Trim();
            char trans[STD_BUFFER_SIZE];
            strcpy(trans, translation.GetCStr());
            transtree->addText(original.GetCStr(), trans);
        }
    }
    fclose(fp);  
    
    if (transtree->text == NULL)
    {
        parse_error = "The ChinaAVG translation file was empty.";
        return false;
    }

    return true;
}

bool parse_chinaavg_untranslation(const char *language_file, String &parse_error) {
    bool isTrans = false;
    char line[STD_BUFFER_SIZE];
    String original, translation;
    FILE *fp;   
    if ((fp = fopen(language_file,"r")) == NULL) {
        parse_error = "Can't open ChinaAVG untranslation file.";
        return false;
    }

    while(!feof(fp)) {
        fgets(line, STD_BUFFER_SIZE, fp);
        if (!isTrans) {
            original = line;
            isTrans = true;
            original.Trim();
        } 
        else {
            isTrans = false;
            untranstree->addText(original.GetCStr(), "");
        }
    }
    fclose(fp);  

    return true;
}
