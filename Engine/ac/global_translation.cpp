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

#include <string.h>
#include "ac/common.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/tree_map.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"

extern GameState play;
extern AGSPlatformDriver *platform;
extern int source_text_length;
extern TreeMap *transtree;
extern TreeMap *untranstree;
extern char transFileName[MAX_PATH];
extern char *untransFileLoc;

bool write_chinaavg_untranslation(const char *language_file, const char *text);

char *get_translation (const char *text) {
    if (text == NULL)
        quit("!Null string supplied to CheckForTranslations");
    // rule out text need not to be translate
    if (strcmp("", text) == 0 || strcmp("%c", text) == 0)
        return (char*)text;
    // get_tanslation is called twice for same text
    // which means game try to translate the translation text
    // i don't know why (i know it now, bu no quick idea to avoid that)
    int i;
    for (i=0; i<strlen(text); i++) {
        if (text[i] == 0)
            break;

        if ((unsigned char)text[i] >= 0x80) {
            return (char*)text;
        }
    }

    source_text_length = strlen(text);
    if ((text[0] == '&') && (play.unfactor_speech_from_textlength != 0)) {
        // if there's an "&12 text" type line, remove "&12 " from the source
        // length
        int j = 0;
        while ((text[j] != ' ') && (text[j] != 0))
            j++;
        j++;
        source_text_length -= j;
    }

    // check if a plugin wants to translate it - if so, return that
    char *plResult = (char*)platform->RunPluginHooks(AGSE_TRANSLATETEXT, (long)text);
    if (plResult) {

//  64bit: This is a wonky way to detect a valid pointer
//  if (((int)plResult >= -1) && ((int)plResult < 10000))
//    quit("!Plugin did not return a string for text translation");

        return plResult;
    }

    if (transtree != NULL) {
        // translate the text using the translation file
        char * transl = transtree->findValue (text);
        if (transl != NULL) {
            if (strlen(transl) > 0)
                return transl;
        }
        else {
            if (untranstree != NULL && untranstree->findValue(text) == NULL) {
                if (write_chinaavg_untranslation(untransFileLoc, (char*)text))
                    untranstree->addText((char*)text, "");
            }
        }
    }
    // return the original text
    return (char*)text;
}

int IsTranslationAvailable () {
    if (transtree != NULL)
        return 1;
    return 0;
}

int GetTranslationName (char* buffer) {
    VALIDATE_STRING (buffer);
    const char *copyFrom = transFileName;

    while (strchr(copyFrom, '\\') != NULL)
    {
        copyFrom = strchr(copyFrom, '\\') + 1;
    }
    while (strchr(copyFrom, '/') != NULL)
    {
        copyFrom = strchr(copyFrom, '/') + 1;
    }

    strcpy (buffer, copyFrom);
    // remove the ".tra" from the end of the filename
    if (strstr (buffer, ".tra") != NULL)
        strstr (buffer, ".tra")[0] = 0;

    return IsTranslationAvailable();
}

bool write_chinaavg_untranslation(const char *language_file, const char *text) {
    FILE *fp; 
    if ((fp = fopen(language_file,"a+")) == NULL) 
        return false;
    
    fputs(text, fp);
    fputs("\n\n", fp);
    fclose(fp);
    return true;
}
