// Embeds the third-party credit strings referenced during engine construction.
// Preserve notice text and single-byte escapes verbatim. [INFERRED] Source placement.
void pcreCreditNotice()
{
    static const char string[] =
        "Regular expression support is provided by the PCRE library package which is open source "
        "software, written by Philip Hazel,and copyright by the University of Cambridge, England. "
        "The source code for PCRE can be found at "
        "ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/";
    (void)string;
}

// pmeCreditNotice - String at 0x8884DA0, 203 bytes.
void pmeCreditNotice()
{
    static const char string[] =
        "This product uses PME, (C) Zachary Hansen, xaxxon@slackworks.com. The PME wrapper was "
        "modified for use in this product. The original source code for PME can be found at  "
        "http://xaxxon.slackworks.com/pme/";
    (void)string;
}

// zlibCreditNotice - String at 0x8884E80, 1,061 bytes.
void zlibCreditNotice()
{
    static const char string[] =
        "zlib.h -- interface of the 'zlib' general purpose compression library  version 1.2.2, "
        "October 3rd, 2004  Copyright (C) 1995-2004 Jean-loup Gailly and Mark Adler  This software "
        "is provided 'as-is', without any express or implied  warranty.  In no event will the "
        "authors be held liable for any damages  arising from the use of this software.  Permission "
        "is granted to anyone to use this software for any purpose,  including commercial "
        "applications, and to alter it and redistribute it  freely, subject to the following "
        "restrictions:  1. The origin of this software must not be misrepresented; you must not     "
        "claim that you wrote the original software. If you use this software     in a product, an "
        "acknowledgment in the product documentation would be     appreciated but is not required.  "
        "2. Altered source versions must be plainly marked as such, and must not be     "
        "misrepresented as being the original software.  3. This notice may not be removed or "
        "altered from any source distribution.  Jean-loup Gailly jloup@gzip.org  Mark Adler "
        "madler@alumni.caltech.edu";
    (void)string;
}

// Preserves the notice's original single-byte copyright and quotation-mark encoding.
void oggVorbisCreditNotice()
{
    static const char string[] =
        "\xA9 2002-2003, Jean-Marc Valin/Xiph.Org FoundationRedistribution and use in source and "
        "binary forms, with or without modification, are permitted provided that the following "
        "conditions are met: Redistributions of source code must retain the above copyright notice, "
        "this list of conditions and the following disclaimer. Redistributions in binary form must "
        "reproduce the above copyright notice, this list of conditions and the following disclaimer "
        "in the documentation and/or other materials provided with the distribution. Neither the "
        "name of the Xiph.org Foundation nor the names of its contributors may be used to endorse "
        "or promote products derived from this software without specific prior written permission. "
        "This software is provided by the copyright holders and contributors \x93""as is\x94"" and "
        "any express or implied warranties, including, but not limited to, the implied warranties "
        "of merchantability and fitness for a particular purpose are disclaimed. In no event shall "
        "the foundation or contributors be liable for any direct, indirect, incidental, special, "
        "exemplary, or consequential damages (including, but not limited to, procurement of "
        "substitute goods or services; loss of use, data, or profits; or business interruption) "
        "however caused and on any theory of liability, whether in contract, strict liability, or "
        "tort (including negligence or otherwise) arising in any way out of the use of this "
        "software, even if advised of the possibility of such damage. ";
    (void)string;
}

// openAlCreditNotice - String at 0x8885880, 398 bytes.
// Also opens with the single 0xA9 (c) byte.
void openAlCreditNotice()
{
    static const char string[] =
        "\xA9 2005 Creative Technology Ltd. All rights reserved. The Creative logo,Sound Blaster, "
        "Sound Blaster logo, Audigy, and EAX are registered trademarks and ADVANCED HD, the EAX "
        "ADVANCED HD logo are trademarks of Creative Technology Ltd. in the United States and/or "
        "other countries. All brands or product names listed are trademarks or registered "
        "trademarks and are property of their respective holders.";
    (void)string;
}
