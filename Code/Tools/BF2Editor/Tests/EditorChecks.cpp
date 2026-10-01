// Code/Tools/BF2Editor/Tests/EditorChecks.cpp

// Focused checks for the editor's framework pieces that need no engine: the command-line parser, the
// source-control registry, the undo history and the selection. Target bf2_editor_checks; run by
// bf2_editor_run_checks. Editor-only code (D3). Prints one line per failure and a summary; the exit
// code is the failure count.
#include "Framework/EditorConfig.h"

#include "App/CommandLine.h"
#include "Framework/AnimationSystemFile.h"
#include "Framework/BafHeader.h"
#include "Framework/GameplayLayers.h"
#include "Framework/LightSetupFile.h"
#include "Framework/GroundPropertiesFile.h"
#include "Framework/TerrainRawLayers.h"
#include "Framework/DdsImage.h"
#include "Framework/CommandHistory.h"
#include "Framework/DescFile.h"
#include "Framework/ConsoleCompletion.h"
#include "Framework/EditorSettings.h"
#include "Framework/HelpDocuments.h"
#include "Framework/LayerSummary.h"
#include "Framework/LevelCopier.h"
#include "Framework/LevelEditorDefs.h"
#include "Framework/LevelPacker.h"
#include "Framework/LevelObjectFile.h"
#include "Framework/LevelSettingsFile.h"
#include "Framework/MapDescription.h"
#include "Framework/ModScaffold.h"
#include "Framework/ObjectArchives.h"
#include "Framework/ObjectLibrary.h"
#include "Framework/TemplateClone.h"
#include "Framework/WizardRules.h"
#include "Framework/MaterialFiles.h"
#include "Framework/TemplateFile.h"
#include "Framework/Matrix4.h"
#include "Framework/Selection.h"
#include "Framework/Shortcuts.h"
#include "Framework/SourceControl.h"
#include "Framework/UndergrowthConfig.h"
#include "Framework/UndergrowthMap.h"
#include "Framework/ZipWriter.h"

#include "unzip.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace bf2editor;

namespace
{

int g_passed = 0;
int g_failed = 0;

void check(bool condition, const char* what, int line)
{
    if (condition)
    {
        ++g_passed;
        return;
    }
    ++g_failed;
    std::printf("FAIL line %d: %s\n", line, what);
}

#define CHECK(condition) check((condition), #condition, __LINE__)

bool hasText(const std::vector<std::string>& lines, const char* text)
{
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].find(text) != std::string::npos)
            return true;
    }
    return false;
}

void checkTokenizer()
{
    std::string error;
    std::vector<std::string> t = tokenizeCommandLine("  +a  b \"c d\" \"\" +e\t", error);
    CHECK(error.empty());
    CHECK(t.size() == 5);
    CHECK(t.size() == 5 && t[0] == "+a" && t[1] == "b" && t[2] == "c d" && t[3].empty() && t[4] == "+e");

    t = tokenizeCommandLine("+runConFileAndClose \"C:\\a b\\x.con", error);
    CHECK(!error.empty());
}

void checkGoldenLines()
{
    // Match the retail editor's recorded command output.
    EditorOptions o = parseCommandLine(
        "+loadMod bf2 +enableAsserts 0 +seh 1 +skipBF2ContentCheck 1 +saveAndClose Strike_at_Karkand");
    CHECK(o.errors.empty());
    CHECK(o.warnings.empty());
    CHECK(o.mod == "bf2");
    CHECK(o.level == "Strike_at_Karkand");
    CHECK(o.batchSaveParts == SaveAll);
    CHECK(o.enableAsserts == 0);
    CHECK(o.seh == 1);
    CHECK(o.skipContentCheck == 1);
    CHECK(o.isBatch());

    // G6: a script run from the mod root.
    o = parseCommandLine("+loadMod bf2 +loadLevel Strike_at_Karkand +runConFileAndClose "
                         "\"D:\\game dir\\mods\\bf2\\_wp1_G6_place_object.con\"");
    CHECK(o.errors.empty());
    CHECK(o.runConFileAndClose == "D:\\game dir\\mods\\bf2\\_wp1_G6_place_object.con");
    CHECK(o.batchSaveParts == 0);
    CHECK(o.isBatch());

    // dice's navmesh pipeline line (WP1 readme).
    o = parseCommandLine("+loadMod bf2 +loadLevel Dalian_Plant +runConFileAndClose \"nav.con\" "
                         "+enableAsserts 0 +forceLoadPlugin SinglePlayerEditor");
    CHECK(o.errors.empty());
    CHECK(o.forceLoadModules.size() == 1 && o.forceLoadModules[0] == "SinglePlayerEditor");
}

void checkSwitchRules()
{
    // Names are not case-sensitive, as in retail.
    EditorOptions o = parseCommandLine("+LOADMOD bf2 +loadlevel X +SAVEOBJECTSANDCLOSE x");
    CHECK(o.errors.empty());
    CHECK(o.level == "X");
    CHECK(o.batchSaveParts == SaveObjects);

    // Save switches combine.
    o = parseCommandLine("+saveTerrainAndClose L +saveRoadAndClose L +saveOvergrowthAndClose L");
    CHECK(o.errors.empty());
    CHECK(o.batchSaveParts == (SaveTerrain | SaveRoads | SaveOvergrowth));

    // Two different levels in one run are refused.
    o = parseCommandLine("+loadLevel A +saveAndClose B");
    CHECK(hasText(o.errors, "already named"));

    // A save switch needs its level.
    o = parseCommandLine("+saveAndClose +loadMod bf2");
    CHECK(hasText(o.errors, "needs a level name"));

    // Bare +ignoreAsserts disables asserts; "+ignoreAsserts 0" enables them.
    o = parseCommandLine("+ignoreAsserts");
    CHECK(o.errors.empty() && o.enableAsserts == 0);
    o = parseCommandLine("+ignoreAsserts 0");
    CHECK(o.errors.empty() && o.enableAsserts == 1);

    // 0|1 switches refuse anything else.
    o = parseCommandLine("+enableAsserts yes");
    CHECK(hasText(o.errors, "takes 0 or 1"));

    // Archive use can be turned off (retail could not).
    o = parseCommandLine("+useZipArchives 0");
    CHECK(o.errors.empty() && o.useArchives == 0);

    // +customContent gains ".ini" only when it has no extension.
    o = parseCommandLine("+customContent Content");
    CHECK(o.customContent == "Content.ini");
    o = parseCommandLine("+customContent \"my dir.v2\\Content.cfg\"");
    CHECK(o.customContent == "my dir.v2\\Content.cfg");
    o = parseCommandLine("+customContent \"my dir.v2\\Content\"");
    CHECK(o.customContent == "my dir.v2\\Content.ini");

    // Renderer options are validated against the retail values and kept in order.
    o = parseCommandLine("+videoOptions HIGH +deviceBehavior 64 +skipMips 2 +rp 4 +deviceType x");
    CHECK(o.errors.empty());
    CHECK(o.rendererOptions.size() == 3);
    CHECK(o.rendererOptions.size() == 3 && o.rendererOptions[0].first == "videoOptions"
          && o.rendererOptions[0].second == "HIGH" && o.rendererOptions[2].second == "2");
    CHECK(hasText(o.warnings, "+rp '4' ignored"));
    CHECK(hasText(o.warnings, "+deviceType 'x' ignored"));

    // Unknown switches and stray words are warnings, not silence.
    o = parseCommandLine("+modPath mods/bf2 stray");
    CHECK(o.errors.empty());
    CHECK(hasText(o.warnings, "unknown switch '+modPath'"));
    CHECK(hasText(o.warnings, "ignored 'stray'"));

    // A '+' inside a quoted value belongs to the value.
    o = parseCommandLine("+runConFileAndClose \"C:\\a+b\\x.con\" +saveObjectsAndClose L");
    CHECK(o.errors.empty());
    CHECK(o.runConFileAndClose == "C:\\a+b\\x.con");
    CHECK(o.batchSaveParts == SaveObjects);
    CHECK(o.warnings.empty());

    // Two scripts in one run are refused.
    o = parseCommandLine("+runConFileAndClose a.con +runConFileAndClose b.con");
    CHECK(hasText(o.errors, "given twice"));

    // +help and +? both ask for the usage text.
    CHECK(parseCommandLine("+?").help);
    CHECK(parseCommandLine("+Help").help);
    CHECK(!usageText().empty());

    // The editor's own switches (WP4).
    o = parseCommandLine("+szx 1600 +szy 900 +frames 5 +windowHidden 1 +screenshot \"C:\\a b\\shot.bmp\"");
    CHECK(o.errors.empty());
    CHECK(o.windowWidth == 1600 && o.windowHeight == 900);
    CHECK(o.maxFrames == 5);
    CHECK(o.windowHidden);
    CHECK(o.screenshot == "C:\\a b\\shot.bmp");
    CHECK(!o.isBatch());
    o = parseCommandLine("+szx 100");
    CHECK(hasText(o.errors, "+szx takes a size"));
    o = parseCommandLine("+frames 0");
    CHECK(hasText(o.errors, "+frames takes a frame count"));
    o = parseCommandLine("+windowHidden 2");
    CHECK(hasText(o.errors, "takes 0 or 1"));
    o = parseCommandLine("");
    CHECK(o.windowWidth == -1 && o.maxFrames == 0 && !o.windowHidden && o.screenshot.empty());
    CHECK(!o.cameraGiven);

    // +cameraAt takes exactly five comma-separated numbers: x,y,z in metres, then yaw and pitch.
    o = parseCommandLine("+cameraAt 190,185.5,-90,35,-8");
    CHECK(o.errors.empty() && o.cameraGiven);
    CHECK(o.camera[0] == 190.0f && o.camera[1] == 185.5f && o.camera[2] == -90.0f);
    CHECK(o.camera[3] == 35.0f && o.camera[4] == -8.0f);
    // The gameplay layer (phase C), and the command line a layer switch relaunches with.
    o = parseCommandLine("+loadLevel Strike_at_Karkand +gameMode gpm_coop +layerSize 16");
    CHECK(o.errors.empty() && o.gameMode == "gpm_coop" && o.layerSize == 16 && o.level == "Strike_at_Karkand");
    CHECK(parseCommandLine("+loadLevel x").gameMode == "gpm_cq" && parseCommandLine("+loadLevel x").layerSize == 64);
    CHECK(hasText(parseCommandLine("+layerSize 48").errors, "+layerSize takes 16, 32, 64 or 128"));
    CHECK(hasText(parseCommandLine("+gameMode gpm/cq").errors, "+gameMode takes a game mode folder name"));
    CHECK(parseCommandLine("+loadMod bf2").commandLine == "+loadMod bf2");
    CHECK(relaunchCommandLine("+loadMod bf2 +loadLevel Strike_at_Karkand +cameraAt 1,2,3,4,5 +gameMode gpm_cq "
                              "+layerSize 64 +szx 1600 +runConFile \"C:/a b/s.con\" +frames 30",
                              "gpm_coop", 16)
          == "+loadMod bf2 +loadLevel Strike_at_Karkand +szx 1600 +gameMode gpm_coop +layerSize 16");
    CHECK(relaunchCommandLine("+loadLevel \"My Level\" +windowHidden", "sp1", 32)
          == "+loadLevel \"My Level\" +windowHidden +gameMode sp1 +layerSize 32");
    o = parseCommandLine("+cameraAt 1,2,3,4");
    CHECK(hasText(o.errors, "+cameraAt takes x,y,z,yaw,pitch") && !o.cameraGiven);
    o = parseCommandLine("+cameraAt 1,2,3,4,5,6");
    CHECK(hasText(o.errors, "+cameraAt takes x,y,z,yaw,pitch") && !o.cameraGiven);
    o = parseCommandLine("+cameraAt 1,2,3,4,5x");
    CHECK(hasText(o.errors, "+cameraAt takes x,y,z,yaw,pitch"));

    // +runConFile runs a script in an interactive session; it is not the retail batch switch.
    o = parseCommandLine("+loadLevel L +runConFile \"C:\\t\\edit test.con\"");
    CHECK(o.errors.empty() && o.runConFile == "C:\\t\\edit test.con" && o.runConFileAndClose.empty());
    CHECK(!o.isBatch());
    o = parseCommandLine("+runConFile a.con +runConFile b.con");
    CHECK(hasText(o.errors, "+runConFile given twice"));
    o = parseCommandLine("+runConFile \"\"");
    CHECK(hasText(o.errors, "+runConFile needs a script"));

    // +settingsFile names the settings file a test run may use.
    o = parseCommandLine("+windowHidden 1 +settingsFile \"C:\\t\\s.ini\"");
    CHECK(o.errors.empty() && o.settingsFile == "C:\\t\\s.ini" && o.windowHidden);
    o = parseCommandLine("+settingsFile \"\"");
    CHECK(hasText(o.errors, "+settingsFile needs a file name"));
}

class TestProvider : public ISourceControl
{
public:
    const char*  name() const override { return "Test"; }
    bool         isConnected() const override { return true; }
    ScmFileState getState(const std::string&) override { return ScmFileState::Unchanged; }
    ScmResult    openForEdit(const std::string&) override { return ScmResult{ true, std::string() }; }
    ScmResult    add(const std::string&) override { return ScmResult{ true, std::string() }; }
    ScmResult    remove(const std::string&) override { return ScmResult{ true, std::string() }; }
    ScmResult    revert(const std::string&) override { return ScmResult{ true, std::string() }; }
};

std::unique_ptr<ISourceControl> createTest()
{
    return std::unique_ptr<ISourceControl>(new TestProvider());
}

std::unique_ptr<ISourceControl> createNothing()
{
    return std::unique_ptr<ISourceControl>();
}

void checkSourceControl()
{
    // NullSourceControl is registered and active from the start.
    CHECK(std::string(sourceControl().name()) == "Null");
    CHECK(!sourceControl().isConnected());
    CHECK(sourceControlNames().size() == 1);

    NullSourceControl null;
    CHECK(null.getState("anything") == ScmFileState::NotControlled);
    CHECK(null.add("x").ok);
    CHECK(null.remove("x").ok);
    CHECK(!null.revert("x").ok);

    char directory[MAX_PATH];
    char path[MAX_PATH];
    CHECK(GetTempPathA(MAX_PATH, directory) != 0);
    CHECK(GetTempFileNameA(directory, "bfe", 0, path) != 0);      // creates an empty, writable file
    CHECK(null.openForEdit(path).ok);
    CHECK(SetFileAttributesA(path, FILE_ATTRIBUTE_READONLY) != 0);
    const ScmResult readOnly = null.openForEdit(path);
    CHECK(!readOnly.ok && readOnly.message.find("read-only") != std::string::npos);
    CHECK(SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL) != 0);
    CHECK(DeleteFileA(path) != 0);
    CHECK(null.openForEdit(path).ok);                             // missing: a new file
    CHECK(!null.openForEdit(directory).ok);                       // a directory is not a file

    // Other providers register by name and are selected by name; unknown names change nothing.
    CHECK(registerSourceControl("Test", &createTest));
    CHECK(!registerSourceControl("test", &createTest));           // names are not case-sensitive
    CHECK(!registerSourceControl("Other", 0));
    CHECK(registerSourceControl("Broken", &createNothing));
    CHECK(!selectSourceControl("Perforce"));
    CHECK(std::string(sourceControl().name()) == "Null");
    CHECK(!selectSourceControl("Broken"));                        // a null provider is refused
    CHECK(std::string(sourceControl().name()) == "Null");
    CHECK(selectSourceControl("TEST"));
    CHECK(std::string(sourceControl().name()) == "Test" && sourceControl().isConnected());
    CHECK(selectSourceControl("null"));
    CHECK(std::string(sourceControl().name()) == "Null");
    CHECK(sourceControlNames().size() == 3);
}

// A console stand-in for the history: it records every script and fails any that contains "FAIL".
bool recordingRunner(void* context, const std::string& script, std::string& error)
{
    static_cast<std::vector<std::string>*>(context)->push_back(script);
    if (script.find("FAIL") != std::string::npos)
    {
        error = "refused: " + script;
        return false;
    }
    return true;
}

HistoryEntry entry(const char* name)
{
    HistoryEntry e;
    e.label = name;
    e.doScript = std::string("do ") + name;
    e.undoScript = std::string("undo ") + name;
    return e;
}

void checkHistory()
{
    std::string error;
    CommandHistory noRunner;
    CHECK(!noRunner.execute(entry("a"), error) && !noRunner.canUndo());   // no console: nothing runs

    std::vector<std::string> ran;
    CommandHistory h(3);
    h.setRunner(&recordingRunner, &ran);
    CHECK(!h.isDirty() && !h.canUndo() && !h.canRedo() && !h.undo(error) && !h.redo(error));

    // Execute runs the do script, then records; undo and redo run the matching halves.
    CHECK(h.execute(entry("a"), error) && h.execute(entry("b"), error));
    CHECK(ran.size() == 2 && ran[0] == "do a" && ran[1] == "do b");
    CHECK(h.isDirty() && h.position() == 2 && h.nextUndo() && h.nextUndo()->label == "b");
    CHECK(h.undo(error) && ran.back() == "undo b" && h.position() == 1 && h.canRedo());
    CHECK(h.redo(error) && ran.back() == "do b" && h.position() == 2 && !h.canRedo());

    // A failing do script records nothing and keeps the redo tail.
    CHECK(h.undo(error));
    const size_t before = h.entries().size();
    CHECK(!h.execute(entry("FAIL"), error) && error.find("refused") != std::string::npos);
    CHECK(h.entries().size() == before && h.position() == 1 && h.canRedo());

    // A successful one drops the redo tail.
    CHECK(h.execute(entry("c"), error) && h.entries().size() == 2 && !h.canRedo());
    CHECK(h.entries().size() == 2 && h.entries()[1].label == "c");

    // A failing undo leaves the position alone.
    CommandHistory f;
    std::vector<std::string> ranF;
    f.setRunner(&recordingRunner, &ranF);
    HistoryEntry bad = entry("x");
    bad.undoScript = "FAIL x";
    CHECK(f.execute(bad, error) && !f.undo(error) && f.position() == 1);

    // Dirty state follows the saved position, through undo and redo.
    h.markSaved();
    CHECK(!h.isDirty());
    CHECK(h.undo(error) && h.isDirty());
    CHECK(h.redo(error) && !h.isDirty());

    // Saving, undoing, then making a different change: the saved state is unreachable for good.
    CHECK(h.undo(error) && h.execute(entry("d"), error) && h.isDirty());
    CHECK(h.undo(error) && h.isDirty());

    // The limit drops the oldest entry.
    CommandHistory small(2);
    std::vector<std::string> ranS;
    small.setRunner(&recordingRunner, &ranS);
    CHECK(small.execute(entry("1"), error) && small.execute(entry("2"), error) && small.execute(entry("3"), error));
    CHECK(small.entries().size() == 2 && small.entries()[0].label == "2" && small.position() == 2);
    CHECK(small.undo(error) && small.undo(error) && !small.undo(error));
    CHECK(small.isDirty());     // the unchanged level lies before the dropped entry

    // An entry recorded after the fact runs nothing now, and undoes and redoes like any other.
    CommandHistory made;
    std::vector<std::string> ranM;
    made.setRunner(&recordingRunner, &ranM);
    made.record(entry("create"));
    CHECK(ranM.empty() && made.position() == 1 && made.isDirty());
    CHECK(made.undo(error) && ranM.back() == "undo create" && made.redo(error) && ranM.back() == "do create");

    // Clearing forgets the entries and keeps the dirty state.
    const unsigned revision = small.revision();
    small.clear();
    CHECK(small.entries().empty() && small.position() == 0 && small.isDirty() && small.revision() != revision);
    CommandHistory clean;
    clean.clear();
    CHECK(!clean.isDirty());
}

void checkSelection()
{
    Selection s;
    CHECK(s.empty() && s.primary() == 0);
    const unsigned r0 = s.revision();
    s.set(7);
    CHECK(s.size() == 1 && s.primary() == 7 && s.contains(7) && s.revision() != r0);
    const unsigned r1 = s.revision();
    s.set(7);
    CHECK(s.revision() == r1);                  // no change, no new revision
    s.add(9);
    s.add(0);                                   // 0 is never an object
    CHECK(s.size() == 2 && s.primary() == 9);
    s.add(7);                                   // re-adding moves it to primary
    CHECK(s.size() == 2 && s.primary() == 7 && s.ids()[0] == 9);
    s.toggle(9);
    CHECK(s.size() == 1 && !s.contains(9));
    s.toggle(9);
    CHECK(s.contains(9) && s.primary() == 9);
    s.remove(42);
    CHECK(s.size() == 2);
    s.set(0);
    CHECK(s.empty() && s.primary() == 0);
}

bool closeTo(float a, float b)
{
    const float d = a - b;
    return d < 1e-4f && d > -1e-4f;
}

void checkMatrices()
{
    const float identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    // Scaled, rotated 90 degrees about Y (row vectors: +X goes to -Z), translated.
    const float m[16] = { 0, 0, -2, 0, 0, 3, 0, 0, 4, 0, 0, 0, 5, 6, 7, 1 };
    float inverse[16], product[16];
    CHECK(matrixInvertAffine(m, inverse));
    matrixMultiply(m, inverse, product);
    bool isIdentity = true;
    for (int i = 0; i < 16; ++i)
        isIdentity = isIdentity && closeTo(product[i], identity[i]);
    CHECK(isIdentity);
    const float singular[16] = { 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    CHECK(!matrixInvertAffine(singular, inverse));

    // A group follows its pivot: a member 2 m along +X from the pivot, the pivot turned 90 degrees
    // about Y in place, ends 2 m along -Z from it, turned the same way.
    const float pivotStart[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 0, 0, 1 };
    const float pivot[16] = { 0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 10, 0, 0, 1 };
    const float member[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 12, 0, 0, 1 };
    float moved[16];
    CHECK(matrixFollowPivot(member, pivotStart, pivot, moved));
    CHECK(closeTo(moved[12], 10.0f) && closeTo(moved[13], 0.0f) && closeTo(moved[14], -2.0f));
    CHECK(closeTo(moved[0], 0.0f) && closeTo(moved[2], -1.0f) && closeTo(moved[8], 1.0f));
    // A pivot that has not moved moves nothing; a moved one carries every member the same way.
    CHECK(matrixFollowPivot(member, pivotStart, pivotStart, moved) && closeTo(moved[12], 12.0f) && closeTo(moved[0], 1.0f));
    const float lifted[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 5, 0, 1 };
    CHECK(matrixFollowPivot(member, pivotStart, lifted, moved) && closeTo(moved[12], 12.0f) && closeTo(moved[13], 5.0f));
}

ObjectPlacement placementOf(const LevelObjectFile& file, size_t record)
{
    return file.placement(record);
}

void checkLevelObjectFile()
{
    const std::string text =
        "if v_arg1 == BF2Editor\r\nrun /objects/a.con\r\nendIf\r\n\r\n"
        "rem *** alpha ***\r\nObject.create alpha\r\nObject.absolutePosition 1.000/2.000/3.000\r\n"
        "Object.rotation -180.0/0.0/0.0\r\nObject.layer 1\r\n\r\n"
        "rem *** beta ***\r\nObject.create beta\r\nObject.absolutePosition -4.500/5.250/6.125\r\n"
        "Object.layer 2\r\nObject.setLightSourceMask 3\r\n\r\n"
        "object.create gamma\r\nobject.absolutePosition 7/8/9\r\n\r\n";
    LevelObjectFile file;
    file.parse(text);
    CHECK(file.recordCount() == 3 && file.usesCrlf());
    CHECK(file.placement(0).templateName == "alpha" && file.placement(1).templateName == "beta"
          && file.placement(2).templateName == "gamma");
    CHECK(file.placement(0).rotation[0] == -180.0f && file.placement(0).layer == 1 && file.placement(0).hasLayer);
    CHECK(file.placement(1).position[2] == 6.125f && file.placement(1).layer == 2 && !file.placement(2).hasLayer);
    CHECK(file.render() == text && !file.hasChanges());             // untouched: byte for byte

    // The same rotation read back as +180 (as the engine may) is no edit.
    ObjectPlacement p = placementOf(file, 0);
    p.rotation[0] = 180.0f;
    file.setPlacement(0, p);
    CHECK(!file.isEdited(0) && file.render() == text);

    // An edit rewrites only its own lines, in the retail form; other lines stay.
    p = placementOf(file, 1);
    p.position[0] = 0.5f;
    file.setPlacement(1, p);
    std::string expected = text;
    expected.replace(expected.find("-4.500/5.250/6.125"), 18, "0.500/5.250/6.125");
    CHECK(file.isEdited(1) && file.hasChanges() && file.render() == expected);
    p.rotation[0] = 90.0f;                                          // a rotation line appears after the position
    file.setPlacement(1, p);
    expected.replace(expected.find("0.500/5.250/6.125\r\n") + 19, 0, "Object.rotation 90.0/0.0/0.0\r\n");
    CHECK(file.render() == expected);

    // A zero rotation drops the line; a changed layer is rewritten.
    p = placementOf(file, 0);
    p.rotation[0] = 0.0f;
    p.layer = 4;
    file.setPlacement(0, p);
    expected.replace(expected.find("Object.rotation -180.0/0.0/0.0\r\n"), 32, "");
    expected.replace(expected.find("Object.layer 1"), 14, "Object.layer 4");
    CHECK(file.render() == expected);

    // A new record goes last, in the retail form; a deleted one leaves with its separator.
    ObjectPlacement added;
    added.templateName = "delta";
    added.position[0] = 10.0f;
    added.position[1] = -0.0001f;
    added.position[2] = 3.25f;
    added.rotation[0] = 90.0f;
    added.layer = 1;
    added.hasLayer = true;
    const size_t fresh = file.appendRecord(added);
    CHECK(fresh == 3 && file.isNew(fresh));
    expected += "rem *** delta ***\r\nObject.create delta\r\nObject.absolutePosition 10.000/0.000/3.250\r\n"
                "Object.rotation 90.0/0.0/0.0\r\nObject.layer 1\r\n\r\n";
    CHECK(file.render() == expected);
    file.setDeleted(2, true);
    expected.replace(expected.find("object.create gamma"), 54, "");     // its two lines and the blank one
    CHECK(file.render() == expected);
    file.setDeleted(fresh, true);
    file.setDeleted(2, false);

    // Formats: no negative zero, zero rotations recognised, the console's a/b/c form.
    const float nearZero[3] = { -0.0f, 0.04f, -0.04f };
    CHECK(LevelObjectFile::formatRotation(nearZero) == "0.0/0.0/0.0" && LevelObjectFile::rotationPrintsAsZero(nearZero));
    float triple[3];
    CHECK(LevelObjectFile::parseTriple("1.5/-2/3e1", triple) && triple[0] == 1.5f && triple[1] == -2.0f && triple[2] == 30.0f);
    CHECK(!LevelObjectFile::parseTriple("1/2", triple) && !LevelObjectFile::parseTriple("1/2/3/4", triple));

    // LF, no final newline: kept; a new record starts on a line of its own after a blank one.
    LevelObjectFile lf;
    lf.parse("Object.create a\nObject.absolutePosition 0/0/0");
    CHECK(!lf.usesCrlf() && lf.recordCount() == 1 && lf.render() == "Object.create a\nObject.absolutePosition 0/0/0");
    ObjectPlacement b;
    b.templateName = "b";
    lf.appendRecord(b);
    CHECK(lf.render() == "Object.create a\nObject.absolutePosition 0/0/0\n\nrem *** b ***\nObject.create b\n"
                         "Object.absolutePosition 0.000/0.000/0.000\n\n");

    // Binding: same template (case ignored), nearest unused object within the tolerance.
    std::vector<SceneObject> objects(4);
    const char* names[4] = { "ALPHA", "beta", "beta", "gamma" };
    const float positions[4][3] = { { 1, 2, 3 }, { 99, 5.25f, 6.125f }, { -4.5f, 5.25f, 6.125f }, { 7, 8, 9.5f } };
    for (int i = 0; i < 4; ++i)
    {
        objects[i].id = 10 + i;
        objects[i].templateName = names[i];
        for (int k = 0; k < 3; ++k)
            objects[i].position[k] = positions[i][k];
    }
    LevelObjectFile fresh2;
    fresh2.parse(text);
    std::vector<unsigned> ids = bindRecords(fresh2, objects, 0.01f);
    CHECK(ids.size() == 3 && ids[0] == 10 && ids[1] == 12 && ids[2] == 0);     // gamma is 0.5 m off
    ids = bindRecords(fresh2, objects, 1.0f);
    CHECK(ids[2] == 13);

    // The editor-only preload block: found, searched without regard to case, extended before endIf.
    CHECK(fresh2.hasPreloadBlock() && fresh2.preloads("RUN /Objects/A.con ") && !fresh2.preloads("run /objects/b.con"));
    fresh2.setExtraPreload(std::vector<std::string>(1, "run /objects/b.con"));
    CHECK(fresh2.hasChanges() && fresh2.preloads("run /objects/b.con"));
    std::string withPreload = text;
    withPreload.replace(withPreload.find("endIf"), 0, "run /objects/b.con\r\n");
    CHECK(fresh2.render() == withPreload);
    fresh2.setExtraPreload(std::vector<std::string>());
    CHECK(fresh2.render() == text && !fresh2.hasChanges());
    LevelObjectFile noBlock;
    noBlock.parse("if v_arg1 == BF2Editor\nObject.create x\nendIf\n");
    CHECK(!noBlock.hasPreloadBlock());                  // an object inside is not a preload block
}

// A gameplay layer's style (WP6 part 2): three-decimal rotations, zero rotations kept, indented
// records inside host blocks, and a control point with no rotation line.
void checkLevelObjectFileStyles()
{
    LevelObjectFile none;
    none.parse("rem nothing here\r\n");
    CHECK(none.style().positionDecimals == 3 && none.style().rotationDecimals == 1 && !none.style().keepZeroRotation);

    const std::string text =
        "if v_arg1 == host\r\n\r\n"
        "   rem [ObjectSpawner: sp_1]\r\n   Object.create sp_1\r\n"
        "   Object.absolutePosition -172.823/161.298/-309.137\r\n   Object.rotation 90.000/0.000/0.000\r\n"
        "   Object.setControlPointId 13\r\n   Object.layer 3\r\n"
        "endIf\r\n\r\n"
        "rem [SpawnPoint: pt_1]\r\nObject.create pt_1\r\nObject.absolutePosition 1.000/2.000/3.000\r\n"
        "Object.rotation 0.000/0.000/0.000\r\nObject.setControlPointId 13\r\nObject.layer 1\r\n\r\n"
        "   Object.create cp_1\r\n   Object.absolutePosition 4.000/5.000/6.000\r\n   Object.layer 1\r\n";
    LevelObjectFile file;
    file.parse(text);
    CHECK(file.recordCount() == 3 && file.render() == text && !file.hasChanges());
    CHECK(file.style().positionDecimals == 3 && file.style().rotationDecimals == 3 && file.style().keepZeroRotation);

    // An indented record keeps its indentation.
    ObjectPlacement p = placementOf(file, 0);
    p.position[0] = -170.0f;
    file.setPlacement(0, p);
    std::string expected = text;
    expected.replace(expected.find("-172.823/"), 9, "-170.000/");
    CHECK(file.render() == expected);

    // Three decimals: a rotation within half a thousandth is no edit; beyond it is, printed that way.
    p = placementOf(file, 1);
    p.rotation[0] = 0.0004f;
    file.setPlacement(1, p);
    CHECK(!file.isEdited(1) && file.render() == expected);
    p.rotation[0] = 0.25f;
    file.setPlacement(1, p);
    std::string turned = expected;
    turned.replace(turned.find("Object.rotation 0.000/0.000/0.000"), 33, "Object.rotation 0.250/0.000/0.000");
    CHECK(file.isEdited(1) && file.render() == turned);
    // Back to zero with a move: the zero rotation line stays, as this file writes them.
    p.rotation[0] = 0.0f;
    p.position[1] = 2.5f;
    file.setPlacement(1, p);
    expected.replace(expected.find("1.000/2.000/3.000"), 17, "1.000/2.500/3.000");
    CHECK(file.render() == expected);

    // A record with no rotation line gets none for a zero rotation; a real one goes after the
    // position, at the record's indentation.
    p = placementOf(file, 2);
    p.position[2] = 7.0f;
    file.setPlacement(2, p);
    expected.replace(expected.find("4.000/5.000/6.000"), 17, "4.000/5.000/7.000");
    CHECK(file.render() == expected);
    p.rotation[0] = 45.0f;
    file.setPlacement(2, p);
    std::string rotated = expected;
    rotated.replace(rotated.find("4.000/5.000/7.000\r\n") + 19, 0, "   Object.rotation 45.000/0.000/0.000\r\n");
    CHECK(file.render() == rotated);

    // A new record in this file's style: three decimals, the zero rotation written.
    LevelObjectFile fresh;
    fresh.parse(text);
    ObjectPlacement added;
    added.templateName = "pt_2";
    added.position[0] = 1.0f;
    added.layer = 1;
    added.hasLayer = true;
    fresh.appendRecord(added);
    CHECK(fresh.render() == text + "\r\nrem *** pt_2 ***\r\nObject.create pt_2\r\nObject.absolutePosition 1.000/0.000/0.000\r\n"
                                   "Object.rotation 0.000/0.000/0.000\r\nObject.layer 1\r\n\r\n");
    CHECK(LevelObjectFile::formatRotation(added.rotation, 3) == "0.000/0.000/0.000"
          && LevelObjectFile::rotationPrintsAsZero(added.rotation, 3));
}

// `text` with its one `from` replaced by `to` (a check fails when `from` is not there once).
std::string replacedOnce(const std::string& text, const std::string& from, const std::string& to)
{
    const size_t at = text.find(from);
    CHECK(at != std::string::npos && text.find(from, at + 1) == std::string::npos);
    if (at == std::string::npos)
        return text;
    std::string out = text;
    out.replace(at, from.size(), to);
    return out;
}

// The per-object properties (phase B): parsed, kept as written while unchanged, written in the retail
// order (group, disableChildren, notInAI, isOvergrowth, setLightSourceMask), dropped when off.
void checkLevelObjectFileProperties()
{
    const std::string text =
        "rem *** a ***\r\nObject.create a\r\nObject.absolutePosition 1.000/2.000/3.000\r\nobject.layer 1\r\n"
        "Object.isOvergrowth 1\r\nObject.setLightSourceMask 6\r\n\r\n"
        "rem *** b ***\r\nObject.create b\r\nObject.absolutePosition 4.000/5.000/6.000\r\n"
        "Object.rotation 90.0/0.0/0.0\r\nObject.layer 2\r\nObject.disableChildren 1\r\nObject.group 2\r\n\r\n";
    LevelObjectFile file;
    file.parse(text);
    CHECK(file.recordCount() == 2 && file.render() == text && !file.hasChanges());
    const ObjectPlacement a = file.placement(0);
    CHECK(a.isOvergrowth && a.hasLightSourceMask && a.lightSourceMask == 6 && a.group == 0 && !a.notInAI
          && !a.disableChildren && a.layer == 1);
    const ObjectPlacement b = file.placement(1);
    CHECK(b.disableChildren && b.group == 2 && !b.isOvergrowth && !b.hasLightSourceMask);

    // A move rewrites the position line only; the unchanged lines keep their text (the lower-case
    // layer line too).
    ObjectPlacement p = a;
    p.position[0] = 1.5f;
    file.setPlacement(0, p);
    std::string expected = replacedOnce(text, "1.000/2.000/3.000", "1.500/2.000/3.000");
    CHECK(file.render() == expected);

    // A flag turned off drops its line; the ones turned on go in the retail order after the nearest
    // line before them; a changed mask is rewritten in place.
    p.isOvergrowth = false;
    p.group = 3;
    p.notInAI = true;
    p.lightSourceMask = 4;
    file.setPlacement(0, p);
    expected = replacedOnce(expected, "object.layer 1\r\nObject.isOvergrowth 1\r\nObject.setLightSourceMask 6\r\n",
                            "object.layer 1\r\nObject.group 3\r\nObject.notInAI 1\r\nObject.setLightSourceMask 4\r\n");
    CHECK(file.isEdited(0) && file.render() == expected);
    // Back as loaded: the file as it was.
    file.setPlacement(0, a);
    CHECK(file.render() == text && !file.hasChanges());

    // Record b's lines stand in a different order than the retail one; an edit keeps them where they
    // are and adds a new one after the nearest line before it.
    p = b;
    p.disableChildren = false;
    p.isOvergrowth = true;
    file.setPlacement(1, p);
    const std::string second = replacedOnce(text, "Object.disableChildren 1\r\nObject.group 2\r\n",
                                            "Object.group 2\r\nObject.isOvergrowth 1\r\n");
    CHECK(file.render() == second);

    // A new record writes what is set, in the retail order.
    LevelObjectFile fresh;
    fresh.parse("");
    ObjectPlacement c;
    c.templateName = "c";
    c.position[1] = 5.0f;
    c.layer = 1;
    c.hasLayer = true;
    c.group = 2;
    c.disableChildren = true;
    c.hasLightSourceMask = true;
    c.lightSourceMask = 0;
    fresh.appendRecord(c);
    CHECK(fresh.render() == "rem *** c ***\r\nObject.create c\r\nObject.absolutePosition 0.000/5.000/0.000\r\n"
                            "Object.layer 1\r\nObject.group 2\r\nObject.disableChildren 1\r\n"
                            "Object.setLightSourceMask 0\r\n\r\n");
}

// Template blocks (phase C): found by name, their lines keyed (indexed properties by their index),
// edited in place, added at the block's end, removed, and back to the file when every edit reverts.
void checkLevelObjectFileTemplates()
{
    const std::string text =
        "rem [ObjectSpawnerTemplate: sp_1]\r\nObjectTemplate.create ObjectSpawner sp_1\r\n"
        "ObjectTemplate.activeSafe ObjectSpawner sp_1\r\nObjectTemplate.modifiedByUser dgo\r\n"
        "ObjectTemplate.setObjectTemplate 1 RUTNK_T90\r\nObjectTemplate.setObjectTemplate 2 USTNK_M1A2\r\n"
        "ObjectTemplate.minSpawnDelay 20\r\n\r\n"
        "rem [SoundObjectTemplate: s_1]\r\nObjectTemplate.create Sound s_1\r\n"
        "ObjectTemplate.soundFilename \"common/sound/a b.ogg\"\r\nObjectTemplate.volume 0.9\r\n\r\n"
        "   Object.create sp_1\r\n   Object.absolutePosition 1.000/2.000/3.000\r\n"
        "   Object.rotation 90.000/0.000/0.000\r\n   Object.layer 3\r\n";
    LevelObjectFile file;
    file.parse(text);
    std::string className;
    CHECK(file.findTemplate("SP_1", className) && className == "ObjectSpawner" && !file.findTemplate("x", className));
    std::vector<LevelObjectFile::TemplateLine> lines = file.templateLines("sp_1");
    CHECK(lines.size() == 4 && lines[0].key == "modifiedbyuser" && lines[1].key == "setobjecttemplate 1"
          && lines[2].key == "setobjecttemplate 2" && lines[2].value == "2 USTNK_M1A2" && lines[3].value == "20");
    CHECK(file.render() == text && file.editedTemplates() == 0 && !file.hasChanges());

    // An indexed line is edited by its index; a missing one goes after the block's last line.
    CHECK(file.setTemplateValue("sp_1", "setObjectTemplate", "2 USAPC_LAV25"));
    CHECK(file.setTemplateValue("sp_1", "maxSpawnDelay", "50"));
    std::string expected = replacedOnce(text, "setObjectTemplate 2 USTNK_M1A2", "setObjectTemplate 2 USAPC_LAV25");
    expected = replacedOnce(expected, "ObjectTemplate.minSpawnDelay 20\r\n",
                            "ObjectTemplate.minSpawnDelay 20\r\nObjectTemplate.maxSpawnDelay 50\r\n");
    CHECK(file.render() == expected && file.editedTemplates() == 1 && file.hasChanges());
    lines = file.templateLines("sp_1");
    CHECK(lines.size() == 5 && lines[2].value == "2 USAPC_LAV25" && lines[4].key == "maxspawndelay");

    // Removal by key; the addition still follows where the block ended.
    CHECK(file.setTemplateValue("sp_1", "minspawndelay", ""));
    expected = replacedOnce(expected, "ObjectTemplate.minSpawnDelay 20\r\n", "");
    CHECK(file.render() == expected);

    // Every edit reverted: the file as it was.
    CHECK(file.setTemplateValue("sp_1", "setObjectTemplate", "2 USTNK_M1A2"));
    CHECK(file.setTemplateValue("sp_1", "minSpawnDelay", "20"));
    CHECK(file.setTemplateValue("sp_1", "maxspawndelay", ""));
    CHECK(file.render() == text && file.editedTemplates() == 0 && !file.hasChanges());

    // A control point's block runs on over its "rem -----" separators: its radius is found and edited
    // in place, not added to the block's first part.
    const std::string cp =
        "ObjectTemplate.create ControlPoint cp_1\r\nObjectTemplate.physicsType Mesh\r\nrem -----\r\n"
        "ObjectTemplate.addTemplate flagpole\r\nrem -----\r\nObjectTemplate.radius 10\r\nObjectTemplate.team 2\r\n\r\n";
    LevelObjectFile cpFile;
    cpFile.parse(cp);
    CHECK(cpFile.templateLines("cp_1").size() == 4 && cpFile.setTemplateValue("cp_1", "radius", "25"));
    CHECK(cpFile.render() == replacedOnce(cp, "radius 10", "radius 25"));

    // Supply lines (E15): supplyGroupNeeded is keyed by team and group, so each pair is its own line; a second line
    // of one pair rewrites it (1.50 stops on a duplicate entry); a pair is taken out by its key.
    CHECK(LevelObjectFile::templateKey("supplyGroupNeeded", "1 3 0") == "supplygroupneeded 1 3"
          && LevelObjectFile::templateKey("SupplyGroupNeeded", " 2\t5 1") == "supplygroupneeded 2 5");
    CHECK(cpFile.setTemplateValue("cp_1", "supplyGroupId", "2") && cpFile.setTemplateValue("cp_1", "supplyGroupNeeded", "1 3 0")
          && cpFile.setTemplateValue("cp_1", "supplyGroupNeeded", "2 5 0") && cpFile.setTemplateValue("cp_1", "supplyGroupNeeded", "1 3 1"));
    CHECK(cpFile.render() == replacedOnce(replacedOnce(cp, "radius 10", "radius 25"), "ObjectTemplate.team 2\r\n",
                                          "ObjectTemplate.team 2\r\nObjectTemplate.supplyGroupId 2\r\n"
                                          "ObjectTemplate.supplyGroupNeeded 1 3 1\r\nObjectTemplate.supplyGroupNeeded 2 5 0\r\n"));
    CHECK(cpFile.setTemplateValue("cp_1", "supplyGroupNeeded 1 3", "") && cpFile.setTemplateValue("cp_1", "supplyGroupId", "")
          && cpFile.setTemplateValue("cp_1", "supplyGroupNeeded 2 5", "") && cpFile.render() == replacedOnce(cp, "radius 10", "radius 25"));

    // A quoted value is written as given; another template's block is left alone.
    CHECK(file.setTemplateValue("s_1", "soundFilename", "\"common/sound/c d.ogg\""));
    CHECK(file.render() == replacedOnce(text, "\"common/sound/a b.ogg\"", "\"common/sound/c d.ogg\""));
    CHECK(!file.setTemplateValue("nothing", "volume", "1"));
}

// Making and deleting gameplay objects with their templates (phase C): new blocks go into their class's
// section (spawners' and control points' records inside the host block, spawn points as pairs), a
// deleted object takes its banner along, a deleted template its block; the written order is the order
// a parse finds.
void checkLevelObjectFileGameplay()
{
    const std::string text =
        "rem ********** Object Spawner **********\r\n"
        "rem [ObjectSpawnerTemplate: sp_1]\r\nObjectTemplate.create ObjectSpawner sp_1\r\n"
        "ObjectTemplate.activeSafe ObjectSpawner sp_1\r\nObjectTemplate.setObjectTemplate 1 RUTNK_T90\r\n\r\n"
        "if v_arg1 == host\r\n\r\n"
        "   rem [ObjectSpawner: sp_1]\r\n   Object.create sp_1\r\n   Object.absolutePosition 1.000/2.000/3.000\r\n"
        "   Object.rotation 90.000/0.000/0.000\r\n   Object.setControlPointId 13\r\n   Object.layer 3\r\n\r\n"
        "endIf\r\n\r\n"
        "rem ********** Spawn Points **********\r\n"
        "rem [SpawnPointTemplate: pt_1]\r\nObjectTemplate.create SpawnPoint pt_1\r\n"
        "ObjectTemplate.activeSafe SpawnPoint pt_1\r\nObjectTemplate.setControlPointId 13\r\n\r\n"
        "rem [SpawnPoint: pt_1]\r\nObject.create pt_1\r\nObject.absolutePosition 4.000/5.000/6.000\r\n"
        "Object.rotation 0.000/0.000/0.000\r\nObject.layer 3\r\n\r\n"
        "rem ********** Control Points **********\r\n"
        "rem [ControlPointTemplate: cp_1]\r\nObjectTemplate.create ControlPoint cp_1\r\n"
        "ObjectTemplate.activeSafe ControlPoint cp_1\r\nrem -----\r\nObjectTemplate.addTemplate flagpole\r\nrem -----\r\n"
        "ObjectTemplate.controlPointId 13\r\n\r\n"
        "if v_arg1 == host\r\n\r\n"
        "   rem [ControlPoint: cp_1]\r\n   Object.create cp_1\r\n   Object.absolutePosition 7.000/8.000/9.000\r\n"
        "   Object.layer 3\r\n\r\n"
        "endIf\r\n\r\n"
        "CombatAreaManager.use 1\r\n";
    LevelObjectFile file;
    file.parse(text);
    std::vector<size_t> order;
    CHECK(file.recordCount() == 3 && file.render(&order) == text && !file.hasChanges());
    CHECK(order.size() == 3 && order[0] == 0 && order[1] == 1 && order[2] == 2);
    CHECK(file.placement(0).hasControlPointId && file.placement(0).controlPointId == 13
          && !file.placement(1).hasControlPointId && file.dominantLayer() == 3);

    // A spawner: its template after the last spawner template, its record inside the host block.
    std::vector<std::pair<std::string, std::string> > lines;
    lines.push_back(std::make_pair(std::string("isNotSaveable"), std::string("1")));
    lines.push_back(std::make_pair(std::string("hasMobilePhysics"), std::string("0")));
    CHECK(file.appendTemplate("ObjectSpawner", "sp_2", lines) && !file.appendTemplate("ObjectSpawner", "SP_1", lines));
    ObjectPlacement spawner;
    spawner.templateName = "sp_2";
    spawner.position[0] = 10.0f;
    spawner.position[1] = 2.0f;
    spawner.position[2] = 3.0f;
    spawner.controlPointId = 13;
    spawner.hasControlPointId = true;
    spawner.layer = 3;
    spawner.hasLayer = true;
    spawner.hostOnly = true;
    CHECK(file.appendRecord(spawner) == 3);
    std::string expected = replacedOnce(text, "ObjectTemplate.setObjectTemplate 1 RUTNK_T90\r\n\r\n",
        "ObjectTemplate.setObjectTemplate 1 RUTNK_T90\r\n\r\n"
        "rem [ObjectSpawnerTemplate: sp_2]\r\nObjectTemplate.create ObjectSpawner sp_2\r\n"
        "ObjectTemplate.activeSafe ObjectSpawner sp_2\r\nObjectTemplate.isNotSaveable 1\r\n"
        "ObjectTemplate.hasMobilePhysics 0\r\n\r\n");
    expected = replacedOnce(expected, "   Object.setControlPointId 13\r\n   Object.layer 3\r\n\r\n",
        "   Object.setControlPointId 13\r\n   Object.layer 3\r\n\r\n"
        "   rem [ObjectSpawner: sp_2]\r\n   Object.create sp_2\r\n   Object.absolutePosition 10.000/2.000/3.000\r\n"
        "   Object.rotation 0.000/0.000/0.000\r\n   Object.setControlPointId 13\r\n   Object.layer 3\r\n\r\n");
    CHECK(file.render() == expected && file.editedTemplates() == 1 && file.hasChanges());

    // A spawn point: template and record together, after the last spawn point.
    lines.clear();
    lines.push_back(std::make_pair(std::string("isNotSaveable"), std::string("1")));
    lines.push_back(std::make_pair(std::string("setControlPointId"), std::string("13")));
    CHECK(file.appendTemplate("SpawnPoint", "pt_2", lines));
    ObjectPlacement point;
    point.templateName = "pt_2";
    point.position[0] = 11.0f;
    point.position[1] = 5.0f;
    point.position[2] = 6.0f;
    point.rotation[0] = 45.0f;
    point.layer = 3;
    point.hasLayer = true;
    CHECK(file.appendRecord(point) == 4);
    expected = replacedOnce(expected, "rem ********** Control Points",
        "rem [SpawnPointTemplate: pt_2]\r\nObjectTemplate.create SpawnPoint pt_2\r\n"
        "ObjectTemplate.activeSafe SpawnPoint pt_2\r\nObjectTemplate.isNotSaveable 1\r\n"
        "ObjectTemplate.setControlPointId 13\r\n\r\n"
        "rem [SpawnPoint: pt_2]\r\nObject.create pt_2\r\nObject.absolutePosition 11.000/5.000/6.000\r\n"
        "Object.rotation 45.000/0.000/0.000\r\nObject.layer 3\r\n\r\n"
        "rem ********** Control Points");
    CHECK(file.render() == expected);

    // A control point: its template (with separators written as given) after the last control point
    // template, its record - no zero rotation - inside the second host block.
    lines.clear();
    lines.push_back(std::make_pair(std::string("setNetworkableInfo"), std::string("ControlPointInfo")));
    lines.push_back(std::make_pair(std::string(), std::string("rem -----")));
    lines.push_back(std::make_pair(std::string("addTemplate"), std::string("flagpole")));
    lines.push_back(std::make_pair(std::string(), std::string("rem -----")));
    lines.push_back(std::make_pair(std::string("controlPointId"), std::string("14")));
    CHECK(file.appendTemplate("ControlPoint", "cp_2", lines));
    CHECK(file.templateLines("cp_2").size() == 3 && file.templateLines("cp_2")[2].key == "controlpointid");
    ObjectPlacement cp;
    cp.templateName = "cp_2";
    cp.position[0] = 20.0f;
    cp.position[1] = 8.0f;
    cp.position[2] = 9.0f;
    cp.layer = 3;
    cp.hasLayer = true;
    cp.hostOnly = true;
    cp.omitZeroRotation = true;
    CHECK(file.appendRecord(cp) == 5);
    expected = replacedOnce(expected, "ObjectTemplate.controlPointId 13\r\n\r\n",
        "ObjectTemplate.controlPointId 13\r\n\r\n"
        "rem [ControlPointTemplate: cp_2]\r\nObjectTemplate.create ControlPoint cp_2\r\n"
        "ObjectTemplate.activeSafe ControlPoint cp_2\r\nObjectTemplate.setNetworkableInfo ControlPointInfo\r\n"
        "rem -----\r\nObjectTemplate.addTemplate flagpole\r\nrem -----\r\nObjectTemplate.controlPointId 14\r\n\r\n");
    expected = replacedOnce(expected, "   Object.absolutePosition 7.000/8.000/9.000\r\n   Object.layer 3\r\n\r\n",
        "   Object.absolutePosition 7.000/8.000/9.000\r\n   Object.layer 3\r\n\r\n"
        "   rem [ControlPoint: cp_2]\r\n   Object.create cp_2\r\n   Object.absolutePosition 20.000/8.000/9.000\r\n"
        "   Object.layer 3\r\n\r\n");
    CHECK(file.render(&order) == expected && file.editedTemplates() == 3);
    // The written order, which a parse of the result finds.
    CHECK(order.size() == 6 && order[0] == 0 && order[1] == 3 && order[2] == 1 && order[3] == 4 && order[4] == 2
          && order[5] == 5);
    LevelObjectFile reread;
    reread.parse(expected);
    CHECK(reread.recordCount() == 6 && reread.placement(1).templateName == "sp_2" && reread.placement(3).templateName == "pt_2"
          && reread.placement(5).templateName == "cp_2" && reread.placement(1).controlPointId == 13
          && reread.render() == expected);
    std::string className;
    CHECK(reread.findTemplate("cp_2", className) && className == "ControlPoint" && reread.templateLines("cp_2").size() == 3);

    // New ones deleted again (an undo): the file as it was.
    file.setDeleted(3, true);
    file.setDeleted(4, true);
    file.setDeleted(5, true);
    CHECK(file.setTemplateDeleted("sp_2", true) && file.setTemplateDeleted("pt_2", true)
          && file.setTemplateDeleted("cp_2", true) && file.isTemplateDeleted("SP_2"));
    CHECK(file.render(&order) == text && !file.hasChanges() && file.editedTemplates() == 0 && order.size() == 3);

    // A loaded spawn point deleted with its template: both blocks go, banners and separators too; the
    // section's own banner stays. Back again, the file as it was.
    file.setDeleted(1, true);
    CHECK(file.setTemplateDeleted("pt_1", true));
    CHECK(file.render(&order) == replacedOnce(text,
        "rem [SpawnPointTemplate: pt_1]\r\nObjectTemplate.create SpawnPoint pt_1\r\n"
        "ObjectTemplate.activeSafe SpawnPoint pt_1\r\nObjectTemplate.setControlPointId 13\r\n\r\n"
        "rem [SpawnPoint: pt_1]\r\nObject.create pt_1\r\nObject.absolutePosition 4.000/5.000/6.000\r\n"
        "Object.rotation 0.000/0.000/0.000\r\nObject.layer 3\r\n\r\n", ""));
    CHECK(order.size() == 2 && order[0] == 0 && order[1] == 2 && file.editedTemplates() == 1);
    // A loaded spawner too: the host block stays, empty.
    file.setDeleted(0, true);
    CHECK(file.setTemplateDeleted("sp_1", true));
    std::string both = replacedOnce(text,
        "rem [ObjectSpawnerTemplate: sp_1]\r\nObjectTemplate.create ObjectSpawner sp_1\r\n"
        "ObjectTemplate.activeSafe ObjectSpawner sp_1\r\nObjectTemplate.setObjectTemplate 1 RUTNK_T90\r\n\r\n", "");
    both = replacedOnce(both,
        "   rem [ObjectSpawner: sp_1]\r\n   Object.create sp_1\r\n   Object.absolutePosition 1.000/2.000/3.000\r\n"
        "   Object.rotation 90.000/0.000/0.000\r\n   Object.setControlPointId 13\r\n   Object.layer 3\r\n\r\n", "");
    both = replacedOnce(both,
        "rem [SpawnPointTemplate: pt_1]\r\nObjectTemplate.create SpawnPoint pt_1\r\n"
        "ObjectTemplate.activeSafe SpawnPoint pt_1\r\nObjectTemplate.setControlPointId 13\r\n\r\n"
        "rem [SpawnPoint: pt_1]\r\nObject.create pt_1\r\nObject.absolutePosition 4.000/5.000/6.000\r\n"
        "Object.rotation 0.000/0.000/0.000\r\nObject.layer 3\r\n\r\n", "");
    CHECK(file.render() == both);
    file.setDeleted(0, false);
    file.setDeleted(1, false);
    CHECK(file.setTemplateDeleted("sp_1", false) && file.setTemplateDeleted("pt_1", false));
    CHECK(file.render() == text && !file.hasChanges());

    // A file with no section for them: templates, then records, then host-only records in a new host
    // block, at the end.
    LevelObjectFile bare;
    bare.parse("CombatAreaManager.use 1\r\n");
    lines.clear();
    lines.push_back(std::make_pair(std::string("isNotSaveable"), std::string("1")));
    CHECK(bare.appendTemplate("ObjectSpawner", "sp_9", lines) && bare.appendTemplate("SpawnPoint", "pt_9", lines));
    spawner.templateName = "sp_9";
    spawner.rotation[0] = 90.0f;
    point.templateName = "pt_9";
    CHECK(bare.appendRecord(spawner) == 0 && bare.appendRecord(point) == 1);
    CHECK(bare.render(&order) ==
        "CombatAreaManager.use 1\r\n\r\n"
        "rem [ObjectSpawnerTemplate: sp_9]\r\nObjectTemplate.create ObjectSpawner sp_9\r\n"
        "ObjectTemplate.activeSafe ObjectSpawner sp_9\r\nObjectTemplate.isNotSaveable 1\r\n\r\n"
        "rem [SpawnPointTemplate: pt_9]\r\nObjectTemplate.create SpawnPoint pt_9\r\n"
        "ObjectTemplate.activeSafe SpawnPoint pt_9\r\nObjectTemplate.isNotSaveable 1\r\n\r\n"
        "rem [SpawnPoint: pt_9]\r\nObject.create pt_9\r\nObject.absolutePosition 11.000/5.000/6.000\r\n"
        "Object.rotation 45.0/0.0/0.0\r\nObject.layer 3\r\n\r\n"
        "if v_arg1 == host\r\n\r\n"
        "   rem [ObjectSpawner: sp_9]\r\n   Object.create sp_9\r\n   Object.absolutePosition 10.000/2.000/3.000\r\n"
        "   Object.rotation 90.0/0.0/0.0\r\n   Object.setControlPointId 13\r\n   Object.layer 3\r\n\r\n"
        "endIf\r\n\r\n");
    CHECK(order.size() == 2 && order[0] == 1 && order[1] == 0);

    // AmbientObjects.con's layout: templates, then the class's records indented, with no host block.
    const std::string ambient =
        "rem [TriggerTemplate: t_0]\r\nObjectTemplate.create Trigger t_0\r\nObjectTemplate.activeSafe Trigger t_0\r\n"
        "ObjectTemplate.radius 9\r\n\r\n"
        "rem [TriggerTemplate: t_1]\r\nObjectTemplate.create Trigger t_1\r\nObjectTemplate.activeSafe Trigger t_1\r\n"
        "ObjectTemplate.radius 12\r\n\r\n"
        "   rem [Trigger: t_0]\r\n   Object.create t_0\r\n   Object.absolutePosition 0.000/2.000/3.000\r\n"
        "   Object.rotation 0.000/0.000/0.000\r\n   Object.layer 2\r\n\r\n"
        "   rem [Trigger: t_1]\r\n   Object.create t_1\r\n   Object.absolutePosition 1.000/2.000/3.000\r\n"
        "   Object.rotation 0.000/0.000/0.000\r\n   Object.layer 1\r\n\r\n"
        "rem ********** AmbientEffectArea **********\r\n";
    LevelObjectFile triggers;
    triggers.parse(ambient);
    lines.clear();
    lines.push_back(std::make_pair(std::string("radius"), std::string("5")));
    CHECK(triggers.appendTemplate("Trigger", "t_2", lines));
    ObjectPlacement trigger;
    trigger.templateName = "t_2";
    trigger.position[1] = 7.0f;
    trigger.layer = 1;
    trigger.hasLayer = true;
    CHECK(triggers.appendRecord(trigger) == 2);
    expected = replacedOnce(ambient, "ObjectTemplate.radius 12\r\n\r\n",
        "ObjectTemplate.radius 12\r\n\r\n"
        "rem [TriggerTemplate: t_2]\r\nObjectTemplate.create Trigger t_2\r\nObjectTemplate.activeSafe Trigger t_2\r\n"
        "ObjectTemplate.radius 5\r\n\r\n");
    expected = replacedOnce(expected, "   Object.layer 1\r\n\r\n",
        "   Object.layer 1\r\n\r\n"
        "   rem [Trigger: t_2]\r\n   Object.create t_2\r\n   Object.absolutePosition 0.000/7.000/0.000\r\n"
        "   Object.rotation 0.000/0.000/0.000\r\n   Object.layer 1\r\n\r\n");
    CHECK(triggers.render() == expected);
}

// Combat areas (phase C): blocks and manager lines parsed; an untouched file written back as it was; a
// changed block written again in the stock form, in place; a new one after the last block; a deleted one
// gone with its blank line; manager lines edited in place, and missing ones added where the stock order
// puts them.
void checkLevelObjectFileCombatAreas()
{
    const std::string first =
        "CombatArea.create CA_1\r\nCombatArea.min 0.000000/0.000000\r\nCombatArea.max 0.000000/0.000000\r\n"
        "CombatArea.team 0\r\nCombatArea.vehicles 4\r\nCombatArea.layer 3\r\n";
    const std::string second =
        "CombatArea.create CA_2\r\nCombatArea.min 0.000000/0.000000\r\nCombatArea.max 0.000000/0.000000\r\n"
        "CombatArea.addAreaPoint -168.092102/-353.732849\r\nCombatArea.addAreaPoint -179.912323/-352.144592\r\n"
        "CombatArea.addAreaPoint -194.603058/-345.971497\r\nCombatArea.team 0\r\nCombatArea.vehicles 0\r\n"
        "CombatArea.layer 3\r\nCombatArea.usedByPathFinding 1\r\n";
    const std::string text = "endIf\r\n\r\nCombatAreaManager.use 1\r\nCombatAreaManager.timeAllowedOutside 10.000000\r\n"
                             + first + "\r\n" + second + "\r\n";
    LevelObjectFile file;
    file.parse(text);
    CHECK(file.render() == text && !file.hasChanges() && file.combatAreaCount() == 2 && file.editedCombatAreas() == 0);
    size_t area = 0;
    CHECK(file.findCombatArea("ca_2", area) && area == 1 && !file.findCombatArea("CA_9", area));
    const LevelObjectFile::CombatAreaData loaded = file.combatArea(1);
    CHECK(loaded.points.size() == 3 && loaded.points[0].first == -168.092102f && loaded.points[2].second == -345.971497f
          && loaded.vehicles == 0 && loaded.usedByPathFinding && !loaded.inverted && loaded.layer == 3 && loaded.team == 0);
    CHECK(file.combatArea(0).points.empty() && file.combatArea(0).vehicles == 4);
    const LevelObjectFile::CombatAreaSettings settings = file.combatAreaSettings();
    CHECK(settings.hasUse && settings.use && settings.hasTimeAllowedOutside && settings.timeAllowedOutside == 10.0f
          && !settings.hasDamage);
    // The stock form, line for line.
    CHECK(LevelObjectFile::formatCombatArea(loaded, "\r\n") == second);

    // A point moved: its block written again in place, the other kept.
    LevelObjectFile::CombatAreaData moved = loaded;
    moved.points[1].first = -180.5f;
    CHECK(file.setCombatArea(1, moved));
    CHECK(file.render() == replacedOnce(text, "-179.912323/-352.144592", "-180.500000/-352.144592")
          && file.editedCombatAreas() == 1 && file.hasChanges());
    CHECK(file.setCombatArea(1, loaded) && file.render() == text && !file.hasChanges());

    // A new area goes after the last block; a deleted one leaves with its blank line.
    LevelObjectFile::CombatAreaData fresh;
    fresh.name = "CA_3";
    fresh.points.push_back(std::make_pair(1.0f, 2.0f));
    fresh.points.push_back(std::make_pair(3.0f, 2.0f));
    fresh.points.push_back(std::make_pair(3.0f, 5.0f));
    fresh.layer = 3;
    CHECK(file.appendCombatArea(fresh) == 2 && file.isCombatAreaNew(2));
    file.setCombatAreaDeleted(0, true);
    CHECK(file.render() == replacedOnce(text, first + "\r\n", "") + LevelObjectFile::formatCombatArea(fresh, "\r\n") + "\r\n"
          && file.editedCombatAreas() == 2);
    file.setCombatAreaDeleted(0, false);
    file.setCombatAreaDeleted(2, true);
    CHECK(file.render() == text && !file.hasChanges());

    // The manager: timeAllowedOutside edited in place, damage added before it (the stock order).
    LevelObjectFile::CombatAreaSettings changed = settings;
    changed.timeAllowedOutside = 15.0f;
    changed.hasDamage = true;
    changed.damage = 25.0f;
    file.setCombatAreaSettings(changed);
    CHECK(file.combatAreaSettingsChanged() && file.hasChanges());
    CHECK(file.render() == replacedOnce(text, "CombatAreaManager.timeAllowedOutside 10.000000\r\n",
                                        "CombatAreaManager.damage 25.000000\r\nCombatAreaManager.timeAllowedOutside 15.000000\r\n"));
    file.setCombatAreaSettings(settings);
    CHECK(file.render() == text && !file.hasChanges());

    // A file with neither: the manager lines and the new area at the end.
    LevelObjectFile bare;
    bare.parse("rem x\r\n");
    LevelObjectFile::CombatAreaSettings on;
    on.hasUse = true;
    on.use = true;
    on.hasTimeAllowedOutside = true;
    bare.setCombatAreaSettings(on);
    bare.appendCombatArea(fresh);
    CHECK(bare.render() == "rem x\r\n\r\nCombatAreaManager.use 1\r\nCombatAreaManager.timeAllowedOutside 10.000000\r\n"
                           + LevelObjectFile::formatCombatArea(fresh, "\r\n") + "\r\n");

    // A block with a line the editor does not write is kept as it is.
    LevelObjectFile odd;
    odd.parse("CombatArea.create CA_4\r\nCombatArea.min 0.000000/0.000000\r\nCombatArea.somethingElse 1\r\n");
    CHECK(odd.combatAreaCount() == 1 && !odd.setCombatArea(0, fresh));
}

// Ambient sounds (phase C): a Sound template block goes and comes with its `Sound.addTrigger` line; a new
// one takes the class's banner spelling and its record the editor block's form (no banner).
void checkLevelObjectFileSounds()
{
    const std::string blockA =
        "rem [SoundObjectTemplate: S_A]\r\nObjectTemplate.create Sound S_A\r\nObjectTemplate.activeSafe Sound S_A\r\n"
        "ObjectTemplate.soundFilename \"a.ogg\"\r\nObjectTemplate.position 1/2/3\r\nObjectTemplate.minDistance 14\r\n\r\n"
        "Sound.addTrigger S_A\r\n\r\n";
    const std::string blockB =
        "rem [SoundObjectTemplate: S_B]\r\nObjectTemplate.create Sound S_B\r\nObjectTemplate.activeSafe Sound S_B\r\n"
        "ObjectTemplate.soundFilename \"b.ogg\"\r\nObjectTemplate.position 4/5/6\r\n\r\nSound.addTrigger S_B\r\n\r\n";
    const std::string recordA =
        "Object.create S_A\r\nObject.absolutePosition 1.000/2.000/3.000\r\nObject.rotation 0.000/0.000/0.000\r\n"
        "Object.layer 1\r\n\r\n";
    const std::string recordB =
        "Object.create S_B\r\nObject.absolutePosition 4.000/5.000/6.000\r\nObject.rotation 0.000/0.000/0.000\r\n"
        "Object.layer 1\r\n\r\n";
    const std::string text = "rem ************ AmbientSounds ************\r\n\r\n" + blockA + blockB
                           + "if v_arg1 == BF2Editor\r\n\r\n" + recordA + recordB + "endIf\r\n";
    LevelObjectFile file;
    file.parse(text);
    CHECK(file.render() == text && file.recordCount() == 2);
    const std::vector<std::string> names = file.templateNamesOf("sound");
    CHECK(names.size() == 2 && names[0] == "S_A" && names[1] == "S_B" && file.templateHasTrigger("s_b"));

    // A sound deleted: its block, trigger line and record go.
    CHECK(file.setTemplateDeleted("S_A", true));
    file.setDeleted(0, true);
    CHECK(file.render() == replacedOnce(replacedOnce(text, blockA, ""), recordA, "") && file.templateNamesOf("Sound").size() == 1);
    CHECK(file.setTemplateDeleted("S_A", false));
    file.setDeleted(0, false);
    CHECK(file.render() == text && !file.hasChanges());

    // A new sound: its block and trigger line after the last one, its record at the end of the editor
    // block, without a banner.
    std::vector<std::pair<std::string, std::string> > lines;
    lines.push_back(std::make_pair(std::string("soundFilename"), std::string("\"c.ogg\"")));
    lines.push_back(std::make_pair(std::string("position"), std::string("7/8/9")));
    CHECK(file.appendTemplate("Sound", "S_C", lines, true) && file.templateHasTrigger("S_C"));
    ObjectPlacement sound;
    sound.templateName = "S_C";
    sound.position[0] = 7.0f;
    sound.position[1] = 8.0f;
    sound.position[2] = 9.0f;
    sound.layer = 1;
    sound.hasLayer = true;
    CHECK(file.appendRecord(sound) == 2);
    const std::string blockC =
        "rem [SoundObjectTemplate: S_C]\r\nObjectTemplate.create Sound S_C\r\nObjectTemplate.activeSafe Sound S_C\r\n"
        "ObjectTemplate.soundFilename \"c.ogg\"\r\nObjectTemplate.position 7/8/9\r\n\r\nSound.addTrigger S_C\r\n\r\n";
    const std::string recordC =
        "Object.create S_C\r\nObject.absolutePosition 7.000/8.000/9.000\r\nObject.rotation 0.000/0.000/0.000\r\n"
        "Object.layer 1\r\n\r\n";
    CHECK(file.render() == replacedOnce(replacedOnce(text, blockB, blockB + blockC), recordB, recordB + recordC));
    LevelObjectFile reread;
    reread.parse(file.render());
    CHECK(reread.recordCount() == 3 && reread.templateHasTrigger("S_C") && reread.templateNamesOf("Sound").size() == 3);
    // A moved sound: its position line in place, its record's position line in place.
    CHECK(reread.setTemplateValue("S_B", "position", "40/5/60"));
    ObjectPlacement moved = reread.placement(1);
    moved.position[0] = 40.0f;
    moved.position[2] = 60.0f;
    reread.setPlacement(1, moved);
    CHECK(reread.render() == replacedOnce(replacedOnce(file.render(), "ObjectTemplate.position 4/5/6", "ObjectTemplate.position 40/5/60"),
                                          "4.000/5.000/6.000", "40.000/5.000/60.000"));
}

// Renaming a template the file defines (phase C): its banner, create and activeSafe lines, its sound's
// trigger line and its records' banners and create lines, in place; edits go along; a rename back is the
// file as it was; a name another template has, or one that merely starts the same, is left alone.
void checkLevelObjectFileRenames()
{
    const std::string text =
        "rem [ObjectSpawnerTemplate: sp_1]\r\nObjectTemplate.create ObjectSpawner sp_1\r\n"
        "ObjectTemplate.activeSafe ObjectSpawner sp_1\r\nObjectTemplate.setObjectTemplate 1 RUTNK_T90\r\n\r\n"
        "rem [ObjectSpawnerTemplate: sp_10]\r\nObjectTemplate.create ObjectSpawner sp_10\r\n"
        "ObjectTemplate.activeSafe ObjectSpawner sp_10\r\nObjectTemplate.setObjectTemplate 1 sp_1\r\n\r\n"
        "if v_arg1 == host\r\n\r\n"
        "   rem [ObjectSpawner: sp_1]\r\n   Object.create sp_1\r\n   Object.absolutePosition 1.000/2.000/3.000\r\n"
        "   Object.setControlPointId 13\r\n   Object.layer 3\r\n\r\n"
        "   rem [ObjectSpawner: sp_10]\r\n   Object.create sp_10\r\n   Object.absolutePosition 4.000/5.000/6.000\r\n"
        "   Object.layer 3\r\n\r\n"
        "endIf\r\n";
    LevelObjectFile file;
    file.parse(text);
    CHECK(file.render() == text && !file.hasChanges());
    CHECK(file.renameTemplate("SP_1", "spawner_gas"));
    std::string expected = text;
    expected = replacedOnce(expected, "rem [ObjectSpawnerTemplate: sp_1]", "rem [ObjectSpawnerTemplate: spawner_gas]");
    expected = replacedOnce(expected, "create ObjectSpawner sp_1\r\n", "create ObjectSpawner spawner_gas\r\n");
    expected = replacedOnce(expected, "activeSafe ObjectSpawner sp_1\r\n", "activeSafe ObjectSpawner spawner_gas\r\n");
    expected = replacedOnce(expected, "rem [ObjectSpawner: sp_1]", "rem [ObjectSpawner: spawner_gas]");
    expected = replacedOnce(expected, "Object.create sp_1\r\n", "Object.create spawner_gas\r\n");
    CHECK(file.render() == expected && file.hasChanges() && file.editedTemplates() == 1);
    std::string className;
    CHECK(file.findTemplate("spawner_gas", className) && className == "ObjectSpawner" && !file.findTemplate("sp_1", className));
    CHECK(file.placement(0).templateName == "spawner_gas" && file.placement(1).templateName == "sp_10");
    // Taken names and unknown templates are refused.
    CHECK(!file.renameTemplate("spawner_gas", "SP_10") && !file.renameTemplate("nothing", "x") && !file.renameTemplate("sp_10", ""));

    // An edit made under either name goes along with the rename.
    CHECK(file.setTemplateValue("spawner_gas", "setObjectTemplate", "1 USTNK_M1A2"));
    CHECK(file.render() == replacedOnce(expected, "setObjectTemplate 1 RUTNK_T90", "setObjectTemplate 1 USTNK_M1A2"));
    CHECK(file.renameTemplate("spawner_gas", "sp_1"));
    CHECK(file.render() == replacedOnce(text, "setObjectTemplate 1 RUTNK_T90", "setObjectTemplate 1 USTNK_M1A2"));
    CHECK(file.templateLines("sp_1").size() == 1 && file.templateLines("sp_1")[0].value == "1 USTNK_M1A2");
    CHECK(file.setTemplateValue("sp_1", "setObjectTemplate", "1 RUTNK_T90"));
    CHECK(file.render() == text && !file.hasChanges() && file.editedTemplates() == 0);

    // A reread of a renamed file finds the new name everywhere.
    CHECK(file.renameTemplate("sp_10", "sp_harbour"));
    LevelObjectFile reread;
    reread.parse(file.render());
    CHECK(reread.findTemplate("sp_harbour", className) && reread.recordCount() == 2
          && reread.placement(1).templateName == "sp_harbour" && !reread.hasChanges());

    // A sound: its trigger line and its record (no banner) follow.
    const std::string sounds =
        "rem [SoundObjectTemplate: S_A]\r\nObjectTemplate.create Sound S_A\r\nObjectTemplate.activeSafe Sound S_A\r\n"
        "ObjectTemplate.soundFilename \"a.ogg\"\r\n\r\nSound.addTrigger S_A\r\n\r\n"
        "if v_arg1 == BF2Editor\r\n\r\nObject.create S_A\r\nObject.absolutePosition 1.000/2.000/3.000\r\n"
        "Object.rotation 0.000/0.000/0.000\r\nObject.layer 1\r\n\r\nendIf\r\n";
    LevelObjectFile soundFile;
    soundFile.parse(sounds);
    CHECK(soundFile.renameTemplate("S_A", "S_Harbour"));
    std::string renamed;
    for (size_t at = 0; at < sounds.size();)
    {
        const size_t found = sounds.find(" S_A", at);
        if (found == std::string::npos)
        {
            renamed += sounds.substr(at);
            break;
        }
        renamed += sounds.substr(at, found - at) + " S_Harbour";
        at = found + 4;
    }
    CHECK(soundFile.render() == renamed && soundFile.templateHasTrigger("S_Harbour"));
    CHECK(soundFile.renameTemplate("S_Harbour", "S_A") && soundFile.render() == sounds && !soundFile.hasChanges());
}

// Level settings files (phase D): statements found by command and index words in their branch; an edit
// rewrites one value in place; a missing one goes after its command's others; untouched is byte for byte.
void checkLevelSettingsFile()
{
    const std::string init =
        "rem *** Generated by BF2Editor ***\r\nif v_arg1 == BF2Editor\r\n\r\nrun Heightdata.con\r\nLevelSettings.InitWorld\r\n"
        "\r\nelse\r\n\r\nrun Heightdata.con\r\n\r\nendIf\r\n\r\n"
        "gameLogic.setTeamName 1 \"MEC\"\r\ngameLogic.setTeamName 2 \"US\"\r\n\r\n"
        "gameLogic.setKit 1 0 \"MEC_Specops\" \"mec_light_soldier\"\r\ngameLogic.setKit 2 0 \"US_Specops\" \"us_light_soldier\"\r\n\r\n"
        "gameLogic.setBeforeSpawnCamera -203/157/-27 -28/-12/0\r\n\r\n"
        "if v_arg1 == BF2Editor\r\nLevelSettings.CustomTextureSuffix \"\"\r\nelse\r\ntexturemanager.customTextureSuffix \"\"\r\nendIf\r\n"
        "GameLogic.MaximumLevelViewDistance 140\r\n\r\n"
        "gameLogic.setDefaultNumberOfTicketsEx 16 1 100\r\ngameLogic.setDefaultNumberOfTicketsEx 64 2 330\r\n";
    LevelSettingsFile file;
    file.parse(init);
    CHECK(file.render() == init && !file.hasChanges());
    std::string value;
    CHECK(file.value("gameLogic.setTeamName", "2", LevelSettingsFile::Always, value) && value == "\"US\"");
    CHECK(file.value("GAMELOGIC.setKit", "1 0", LevelSettingsFile::Always, value) && value == "\"MEC_Specops\" \"mec_light_soldier\"");
    CHECK(file.value("gameLogic.setDefaultNumberOfTicketsEx", "64 2", LevelSettingsFile::Always, value) && value == "330");
    CHECK(file.value("texturemanager.customTextureSuffix", "", LevelSettingsFile::GameBranch, value) && value == "\"\"");
    CHECK(file.value("LevelSettings.CustomTextureSuffix", "", LevelSettingsFile::EditorBranch, value)
          && !file.value("LevelSettings.CustomTextureSuffix", "", LevelSettingsFile::Always, value));
    CHECK(!file.value("gameLogic.setTeamName", "3", LevelSettingsFile::Always, value));

    // Edits in place: the command's spelling and index words kept.
    file.setValue("GAMELOGIC.SETTEAMNAME", "1", LevelSettingsFile::Always, "\"Rebels\"");
    file.setValue("gameLogic.setBeforeSpawnCamera", "", LevelSettingsFile::Always, "-150/170/-300 30/-10/0");
    file.setValue("texturemanager.customTextureSuffix", "", LevelSettingsFile::GameBranch, "\"_night\"");
    std::string expected = replacedOnce(init, "gameLogic.setTeamName 1 \"MEC\"", "gameLogic.setTeamName 1 \"Rebels\"");
    expected = replacedOnce(expected, "-203/157/-27 -28/-12/0", "-150/170/-300 30/-10/0");
    expected = replacedOnce(expected, "texturemanager.customTextureSuffix \"\"", "texturemanager.customTextureSuffix \"_night\"");
    CHECK(file.render() == expected && file.hasChanges());
    CHECK(file.value("gameLogic.setTeamName", "1", LevelSettingsFile::Always, value) && value == "\"Rebels\"");

    // A missing statement: after its command's last one; one with no such command, at the end.
    file.setValue("gameLogic.setDefaultNumberOfTicketsEx", "32 1", LevelSettingsFile::Always, "200");
    file.setValue("gameLogic.setTicketLossPerMin", "1", LevelSettingsFile::Always, "12");
    expected = replacedOnce(expected, "gameLogic.setDefaultNumberOfTicketsEx 64 2 330\r\n",
                            "gameLogic.setDefaultNumberOfTicketsEx 64 2 330\r\ngameLogic.setDefaultNumberOfTicketsEx 32 1 200\r\n"
                            "gameLogic.setTicketLossPerMin 1 12\r\n");
    CHECK(file.render() == expected);
    CHECK(file.value("gameLogic.setDefaultNumberOfTicketsEx", "32 1", LevelSettingsFile::Always, value) && value == "200");
    // Setting the added one again changes it; setting values back to the file's: no edit is left.
    file.setValue("gameLogic.setDefaultNumberOfTicketsEx", "32 1", LevelSettingsFile::Always, "210");
    CHECK(file.render().find("setDefaultNumberOfTicketsEx 32 1 210\r\n") != std::string::npos);
    file.revert();
    CHECK(file.render() == init && !file.hasChanges());
    file.setValue("gameLogic.setTeamName", "1", LevelSettingsFile::Always, "\"Rebels\"");
    file.setValue("gameLogic.setTeamName", "1", LevelSettingsFile::Always, "\"MEC\"");
    CHECK(file.render() == init);

    // A statement inside another kind of conditional is none of these (Sky.con's editor-only lines are).
    LevelSettingsFile sky;
    sky.parse("Lightmanager.skycolor 0.80/0.75/0.65\nif v_arg1 == host\nRenderer.fogColor 1/2/3\nendIf\nRenderer.fogColor 163.00/135.00/86.00\n");
    CHECK(sky.value("renderer.fogcolor", "", LevelSettingsFile::Always, value) && value == "163.00/135.00/86.00");
    sky.setValue("Lightmanager.skyColor", "", LevelSettingsFile::Always, "0.5/0.5/0.5");
    CHECK(sky.render() == "Lightmanager.skycolor 0.5/0.5/0.5\nif v_arg1 == host\nRenderer.fogColor 1/2/3\nendIf\n"
                          "Renderer.fogColor 163.00/135.00/86.00\n");
}

// The map description (phase D): the layers it lists; a new size copied from its mode's last line
// without ai="1"; a new mode in an element of its own; a listed layer refused.
void checkMapDescription()
{
    const std::string cq64 = "\t\t\t<maptype players=\"64\" type=\"assault\" locid=\"GAMEMODE_DESCRIPTION_assault\">Unlocalized</maptype>\r\n";
    const std::string coop16 =
        "\t\t\t<maptype ai=\"1\" players=\"16\" type=\"assault\" locid=\"GAMEMODE_DESCRIPTION_COOP_assault\">Unlocalized</maptype>\r\n";
    const std::string desc =
        "<map gsid=\"4\">\r\n\t<name> Strike At Karkand </name>\r\n\t<modes>\r\n\t\t<mode type=\"gpm_cq\">\r\n"
        "\t\t\t<maptype ai=\"1\" players=\"16\" type=\"assault\" locid=\"GAMEMODE_DESCRIPTION_assault\">Unlocalized</maptype>\r\n"
        + cq64 + "\t\t</mode>\r\n\t\t<mode type=\"gpm_coop\">\r\n" + coop16 + "\t\t\t\r\n\t\t</mode>\r\n\t</modes>\r\n</map>\r\n";
    const std::vector<std::pair<std::string, int> > layers = mapDescriptionLayers(desc);
    CHECK(layers.size() == 3 && layers[0].first == "gpm_cq" && layers[0].second == 16 && layers[1].second == 64
          && layers[2].first == "gpm_coop" && layers[2].second == 16);

    std::string x = desc;
    CHECK(addMapDescriptionLayer(x, "gpm_cq", 128));
    CHECK(x == replacedOnce(desc, cq64, cq64 + "\t\t\t<maptype players=\"128\" type=\"assault\" locid=\"GAMEMODE_DESCRIPTION_assault\">"
                                             "Unlocalized</maptype>\r\n"));
    CHECK(!addMapDescriptionLayer(x, "GPM_CQ", 128) && mapDescriptionLayers(x).size() == 4);

    std::string y = desc;
    CHECK(addMapDescriptionLayer(y, "gpm_coop", 32));
    CHECK(y == replacedOnce(desc, coop16, coop16 + "\t\t\t<maptype players=\"32\" type=\"assault\" "
                                                   "locid=\"GAMEMODE_DESCRIPTION_COOP_assault\">Unlocalized</maptype>\r\n"));

    std::string z = desc;
    CHECK(addMapDescriptionLayer(z, "sp2", 16));
    CHECK(z == replacedOnce(desc, "\t</modes>", "\t\t<mode type=\"sp2\">\r\n\t\t\t<maptype players=\"16\" type=\"assault\" "
                                               "locid=\"GAMEMODE_DESCRIPTION_COOP_assault\">Unlocalized</maptype>\r\n"
                                               "\t\t</mode>\r\n\t</modes>"));
    CHECK(mapDescriptionLayers(z).size() == 4 && mapDescriptionLayers(z)[3].first == "sp2");
    std::string none = "<map></map>";
    CHECK(!addMapDescriptionLayer(none, "gpm_cq", 16) && none == "<map></map>");
}

// Comparing gameplay layers (phase C): a layer summarised from its file; control points matched across
// layers by their name without the layer's words, or by where they stand.
void checkLayerSummary()
{
    CHECK(controlPointPlace("CPNAME_SK_64_gasstation") == "cpname_sk_gasstation");
    CHECK(controlPointPlace("CPNAME_SK_SP16_gasstation") == "cpname_sk_gasstation");
    CHECK(controlPointPlace("CPNAME_SK_16_hotel") == controlPointPlace("cpname_sk_32_hotel"));
    CHECK(controlPointPlace("MyFlag") == "myflag" && controlPointPlace("Base_2") == "base");

    const std::string small =
        "rem [ControlPointTemplate: CPNAME_XX_16_hotel]\r\nObjectTemplate.create ControlPoint CPNAME_XX_16_hotel\r\n"
        "ObjectTemplate.radius 5\r\nObjectTemplate.team 1\r\nObjectTemplate.controlPointId 27\r\n\r\n"
        "rem [ControlPointTemplate: CPNAME_XX_16_market]\r\nObjectTemplate.create ControlPoint CPNAME_XX_16_market\r\n"
        "ObjectTemplate.controlPointId 28\r\n\r\n"
        "rem [ObjectSpawnerTemplate: sp_1]\r\nObjectTemplate.create ObjectSpawner sp_1\r\n"
        "ObjectTemplate.setObjectTemplate 1 RUTNK_T90\r\nObjectTemplate.setObjectTemplate 2 USTNK_M1A2\r\n\r\n"
        "if v_arg1 == host\r\n\r\n"
        "   rem [ControlPoint: CPNAME_XX_16_hotel]\r\n   Object.create CPNAME_XX_16_hotel\r\n"
        "   Object.absolutePosition 0.000/10.000/0.000\r\n   Object.layer 5\r\n\r\n"
        "   rem [ControlPoint: CPNAME_XX_16_market]\r\n   Object.create CPNAME_XX_16_market\r\n"
        "   Object.absolutePosition 500.000/10.000/0.000\r\n   Object.layer 5\r\n\r\n"
        "   rem [ObjectSpawner: sp_1]\r\n   Object.create sp_1\r\n   Object.absolutePosition 5.000/10.000/0.000\r\n"
        "   Object.setControlPointId 27\r\n   Object.layer 5\r\n\r\n"
        "endIf\r\n\r\n"
        "CombatArea.create CA_16\r\nCombatArea.min 0.000000/0.000000\r\nCombatArea.max 0.000000/0.000000\r\n"
        "CombatArea.addAreaPoint 0.000000/0.000000\r\nCombatArea.addAreaPoint 10.000000/0.000000\r\n"
        "CombatArea.addAreaPoint 10.000000/10.000000\r\nCombatArea.team 0\r\nCombatArea.vehicles 4\r\nCombatArea.layer 5\r\n";
    LevelObjectFile smallFile;
    smallFile.parse(small);
    const LayerSummary a = summarizeLayer(smallFile, "gpm_cq", 16);
    CHECK(a.controlPoints.size() == 2 && a.controlPoints[0].name == "CPNAME_XX_16_hotel" && a.controlPoints[0].id == 27
          && a.controlPoints[0].team == 1 && a.controlPoints[0].radius == 5.0f && a.controlPoints[1].team == 0
          && a.controlPoints[1].radius == 1.0f && a.controlPoints[1].position[0] == 500.0f);
    CHECK(a.spawners.size() == 1 && a.spawners[0].vehicle[0] == "RUTNK_T90" && a.spawners[0].vehicle[1] == "USTNK_M1A2"
          && a.spawners[0].controlPointId == 27);
    CHECK(a.combatAreas.size() == 1 && a.combatAreas[0].name == "CA_16" && a.combatAreas[0].points.size() == 3);

    // A second layer: the hotel moved 30 m under its own name, a factory of its own, and a flag named
    // its own way 20 m from the market.
    LayerSummary b;
    b.mode = "gpm_cq";
    b.size = 64;
    LayerControlPoint point = a.controlPoints[0];
    point.name = "CPNAME_XX_64_hotel";
    point.place = controlPointPlace(point.name);
    point.position[0] = 30.0f;
    b.controlPoints.push_back(point);
    point.name = "CPNAME_XX_64_factory";
    point.place = controlPointPlace(point.name);
    point.position[0] = -800.0f;
    b.controlPoints.push_back(point);
    point.name = "Bazaar";
    point.place = controlPointPlace(point.name);
    point.position[0] = 520.0f;
    b.controlPoints.push_back(point);
    std::vector<LayerSummary> layers;
    layers.push_back(a);
    layers.push_back(b);
    const std::vector<ControlPointRow> rows = matchControlPoints(layers, 50.0f);
    CHECK(rows.size() == 3);
    CHECK(rows.size() == 3 && rows[0].label == "factory" && rows[0].point[0] == -1 && rows[0].point[1] == 1);
    CHECK(rows.size() == 3 && rows[1].label == "hotel" && rows[1].point[0] == 0 && rows[1].point[1] == 0);
    CHECK(rows.size() == 3 && rows[2].label == "market" && rows[2].point[0] == 1 && rows[2].point[1] == 2);
}

void checkSettings()
{
    std::vector<std::string> problems;
    EditorSettings s;
    s.parse("# comment\n; also a comment\n\ncamera.speed = 12.5\r\n gizmo.mode=2 \nbad line\n= 3\n"
            "panels.log = 0\nlevel.Strike_at_Karkand.camera = 190,185,90,35,-8\nkey with space = 1\n",
            problems);
    CHECK(problems.size() == 3);
    CHECK(s.getFloat("camera.speed", 1.0f) == 12.5f);
    CHECK(s.getInt("gizmo.mode", -1) == 2);
    CHECK(!s.getBool("panels.log", true));
    CHECK(s.get("level.Strike_at_Karkand.camera", "") == "190,185,90,35,-8");
    CHECK(s.getInt("camera.speed", 7) == 7);                // not an int: the fallback
    CHECK(s.getFloat("missing", 3.0f) == 3.0f && !s.has("missing"));
    CHECK(s.getBool("gizmo.mode", true));                    // "2" is not a bool: the fallback

    CHECK(s.setFloat("gizmo.moveSnap", 0.25f) && s.setBool("gizmo.snap", true) && s.setInt("gizmo.space", 1));
    CHECK(!s.set("bad key", "x") && !s.set("", "x") && !s.set("k", "two\nlines"));

    // What is written reads back the same.
    EditorSettings copy;
    std::vector<std::string> none;
    copy.parse(s.serialize(), none);
    CHECK(none.empty() && copy.values() == s.values());
    CHECK(copy.getFloat("gizmo.moveSnap", 0.0f) == 0.25f && copy.getBool("gizmo.snap", false));

    // A file on disk: missing reads as defaults; saved and loaded again it round-trips.
    char directory[MAX_PATH];
    char path[MAX_PATH];
    CHECK(GetTempPathA(MAX_PATH, directory) != 0);
    CHECK(GetTempFileNameA(directory, "bfs", 0, path) != 0);
    CHECK(DeleteFileA(path) != 0);
    EditorSettings fromNothing;
    CHECK(fromNothing.load(path, problems) && fromNothing.values().empty());
    std::string error;
    CHECK(s.save(path, error) && error.empty());
    EditorSettings loaded;
    std::vector<std::string> loadProblems;
    CHECK(loaded.load(path, loadProblems) && loadProblems.empty() && loaded.values() == s.values());
    CHECK(DeleteFileA(path) != 0);
}

} // namespace

// Keyboard shortcuts: chords parse and print, a held key takes no modifiers, clashes are
// found (a plain key on a camera key too), and the settings keep what differs from the defaults.
void checkShortcuts()
{
    KeyChord chord;
    CHECK(parseChord("ctrl+shift+z", chord) && chord.ctrl && chord.shift && !chord.alt && chord.key == "Z");
    CHECK(formatChord(chord) == "Ctrl+Shift+Z");
    CHECK(parseChord("Control + F5", chord) && formatChord(chord) == "Ctrl+F5");
    CHECK(parseChord("", chord) && chord.empty() && parseChord("none", chord) && chord.empty());
    CHECK(!parseChord("Ctrl+", chord) && !parseChord("Shift", chord) && !parseChord("Hyper+Z", chord));
    CHECK(parseChord("Delete", chord) && formatChord(chord) == "Delete");

    Shortcuts table;
    CHECK(table.display("edit.undo") == "Ctrl+Z" && table.display("camera.forward") == "W" && table.isDefault("edit.undo"));
    bool clean = true;
    for (size_t i = 0; i < Shortcuts::actions().size(); ++i)
        clean = clean && table.conflicts(Shortcuts::actions()[i].id).empty();
    CHECK(clean);
    std::string error;
    CHECK(table.set("edit.undo", "Ctrl+U", error) && table.display("edit.undo") == "Ctrl+U" && !table.isDefault("edit.undo"));
    CHECK(!table.set("camera.forward", "Ctrl+W", error) && !table.set("nothing", "X", error) && !table.set("edit.undo", "Ctrl+", error));
    CHECK(table.set("gizmo.move", "W", error) && table.conflicts("gizmo.move").size() == 1
          && table.conflicts("camera.forward").size() == 1);
    CHECK(table.set("gizmo.move", "Ctrl+W", error) && table.conflicts("gizmo.move").empty());
    CHECK(table.set("edit.redo", "Ctrl+U", error) && table.conflicts("edit.undo").size() == 1);

    EditorSettings settings;
    CHECK(table.set("selection.frame", "", error));
    table.save(settings);
    CHECK(settings.get("shortcuts.edit.undo", "") == "Ctrl+U" && settings.get("shortcuts.selection.frame", "") == "none"
          && !settings.has("shortcuts.edit.copy"));
    Shortcuts reread;
    reread.load(settings);
    CHECK(reread.display("edit.undo") == "Ctrl+U" && reread.chord("selection.frame").empty()
          && reread.display("edit.copy") == "Ctrl+C");
    table.resetAll();
    table.save(settings);
    CHECK(settings.get("shortcuts.edit.undo", "") == "Ctrl+Z");
    reread.load(settings);
    CHECK(reread.isDefault("edit.undo") && reread.isDefault("selection.frame"));
}

// Packing a level (Framework/LevelPacker, ZipWriter): the stock split rule on sample paths, and an archive
// written and read back through minizip's unzip - the reader behind the game's archives.
void checkLevelPacking()
{
    CHECK(levelArchiveFor("Init.con") == LevelArchive::Server && levelArchiveFor("gamemodes/gpm_cq/64/GamePlayObjects.con") == LevelArchive::Server);
    CHECK(levelArchiveFor("HeightmapPrimary.raw") == LevelArchive::Server && levelArchiveFor("terraindata.raw") == LevelArchive::Client);
    CHECK(levelArchiveFor("Undergrowth.raw") == LevelArchive::Client && levelArchiveFor("Overgrowth\\OvergrowthShadowmap.raw") == LevelArchive::Server);
    CHECK(levelArchiveFor("lightmaps/Objects/LightmapAtlas.tai") == LevelArchive::Client && levelArchiveFor("UndergrowthAtlas.tai") == LevelArchive::Server);
    CHECK(levelArchiveFor("Colormaps/tx00x00.dds") == LevelArchive::Client && levelArchiveFor("Roads/temp_compiled.mesh") == LevelArchive::Client);
    CHECK(levelArchiveFor("Info/Strike_at_Karkand.desc") == LevelArchive::None && levelArchiveFor("Editor/GroundProperties.con") == LevelArchive::None);
    CHECK(levelArchiveFor("archive.md5") == LevelArchive::None && levelArchiveFor("StaticObjects.con.old") == LevelArchive::None);
    CHECK(levelArchiveFor("map.desc") == LevelArchive::Server && levelArchiveFor("Envmaps/EnvironmentMapInfo.emi") == LevelArchive::Server);

    char folder[MAX_PATH], path[MAX_PATH];
    GetTempPathA(MAX_PATH, folder);
    GetTempFileNameA(folder, "bfz", 0, path);
    std::string text;
    for (int i = 0; i < 200; ++i)
        text += "ObjectTemplate.activeSafe SpawnPoint CPNAME_test\r\n";
    std::vector<unsigned char> noise(4096);
    unsigned int seed = 12345;
    for (size_t i = 0; i < noise.size(); ++i)
    {
        seed = seed * 1103515245u + 12345u;
        noise[i] = static_cast<unsigned char>(seed >> 16);
    }
    ZipWriter zip;
    std::string error;
    CHECK(zip.open(path, error));
    CHECK(zip.add("Init.con", reinterpret_cast<const unsigned char*>(text.data()), text.size(), 0x6000, 0x5B21, error));
    CHECK(zip.add("Colormaps/tx00x00.dds", &noise[0], noise.size(), 0, 0, error));
    CHECK(zip.add("empty.txt", 0, 0, 0, 0, error));
    CHECK(zip.close(error) && zip.entryCount() == 3);

    unzFile unz = unzOpen(path);
    CHECK(unz != 0);
    int entries = 0;
    bool same = true;
    bool methods = true;
    for (int status = unz ? unzGoToFirstFile(unz) : UNZ_END_OF_LIST_OF_FILE; status == UNZ_OK; status = unzGoToNextFile(unz))
    {
        char name[256];
        unz_file_info info;
        unzGetCurrentFileInfo(unz, &info, name, sizeof(name), 0, 0, 0, 0);
        std::vector<char> data(info.uncompressed_size + 1);
        unzOpenCurrentFile(unz);
        const int got = unzReadCurrentFile(unz, &data[0], static_cast<unsigned>(data.size()));
        unzCloseCurrentFile(unz);
        const std::string n = name;
        if (n == "Init.con")
        {
            same = same && got == static_cast<int>(text.size()) && std::memcmp(&data[0], text.data(), text.size()) == 0;
            methods = methods && info.compression_method == 8 && info.dosDate == ((0x5B21UL << 16) | 0x6000UL);
        }
        else if (n == "Colormaps/tx00x00.dds")
        {
            same = same && got == static_cast<int>(noise.size()) && std::memcmp(&data[0], &noise[0], noise.size()) == 0;
            methods = methods && info.compression_method == 0;          // noise does not deflate: stored
        }
        else if (n == "empty.txt")
            same = same && got == 0;
        ++entries;
    }
    if (unz)
        unzClose(unz);
    CHECK(entries == 3 && same && methods);
    DeleteFileA(path);
}

// Undergrowth.cfg (Framework/UndergrowthConfig): a sample in the stock shape is written back byte for byte;
// an edit and a new material land where the 2006 writer puts them; with BF2EDITOR_UNDERGROWTH_SAMPLES set
// to a folder of .cfg files (extract_undergrowth_cfgs.py copies the 22 stock ones), each is written back
// byte for byte too.
void checkUndergrowthConfig()
{
    const std::string text =
        "ViewDistance 30\r\nViewDistanceFadeScale 0.5\r\nViewDistanceHeightScale 0.7\r\nViewDistanceHeight2Scale 0.4\r\n"
        "ViewDistanceStreamingScale 1\r\nPatchSubdivide 15\r\nSwayScale 0.15\r\nLightingScale 2.1\r\nAlphaRef 0.15\r\n\r\n"
        "Material defaultgrass 2\r\n{\r\n\tGeneralHeight 0.3\r\n\r\n\tType grass\r\n\t{\r\n\t\tMesh triangle_grass\r\n"
        "\t\tTexture grass_mix4\r\n\t\tRandomSizeScale 0.3 0.65\r\n\t\tDensity 3\r\n\t\tVariation 0.2\r\n"
        "\t\tTerrainColorScale 1\r\n\t\tTypeSwayScale 0.4\r\n\t\tSkew 0\r\n\t}\r\n}\r\n\r\n"
        "Material gravel 3\r\n{\r\n\tGeneralHeight 0\r\n\r\n}\r\n\r\n"
        "Material flowers 7\r\n{\r\n\tGeneralHeight 0\r\n\r\n\tType flower1\r\n\t{\r\n\t\tCrossSize 1.7 0.8\r\n"
        "\t\tTexture flower\r\n\t\tRandomSizeScale 0.7 0.8\r\n\t\tDensity 0.5\r\n\t\tVariation -1\r\n"
        "\t\tTerrainColorScale 1\r\n\t\tTypeSwayScale 0.3\r\n\t\tSkew 1\r\n\t}\r\n}\r\n\r\n";
    UndergrowthConfig cfg;
    std::string error;
    CHECK(cfg.parse(text, error) && cfg.settings.size() == 9 && cfg.materials.size() == 3);
    CHECK(cfg.render() == text);
    CHECK(cfg.materials[2].types[0].mesh.empty() && cfg.materials[2].types[0].crossSize[0] == 1.7f && cfg.materials[2].types[0].skew);
    CHECK(cfg.materialById(3) && cfg.materialById(3)->name == "gravel" && cfg.freeId() == 1);

    // An edit rewrites its line; a new material goes last, with the writer's blank lines.
    cfg.material("gravel")->generalHeight = 0.25f;
    UndergrowthMaterial reeds;
    reeds.name = "reeds";
    reeds.id = cfg.freeId();
    UndergrowthType reed;
    reed.name = "reed";
    reed.mesh = "reed_mesh";
    reed.texture = "reed_tex";
    reeds.types.push_back(reed);
    cfg.materials.push_back(reeds);
    std::string expected = text;
    expected.replace(expected.find("Material gravel 3\r\n{\r\n\tGeneralHeight 0"), 38, "Material gravel 3\r\n{\r\n\tGeneralHeight 0.25");
    expected += "Material reeds 1\r\n{\r\n\tGeneralHeight 0\r\n\r\n\tType reed\r\n\t{\r\n\t\tMesh reed_mesh\r\n\t\tTexture reed_tex\r\n"
                "\t\tRandomSizeScale 1 1\r\n\t\tDensity 1\r\n\t\tVariation 0\r\n\t\tTerrainColorScale 1\r\n"
                "\t\tTypeSwayScale 1\r\n\t\tSkew 0\r\n\t}\r\n}\r\n\r\n";
    CHECK(cfg.render() == expected);
    CHECK(!cfg.parse("Material x 1\r\n{\r\n\tWhat 1\r\n}\r\n", error));

    // The stock files, when a folder of them is given.
    char folder[MAX_PATH];
    const DWORD n = GetEnvironmentVariableA("BF2EDITOR_UNDERGROWTH_SAMPLES", folder, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return;
    WIN32_FIND_DATAA found;
    HANDLE h = FindFirstFileA((std::string(folder) + "\\*.cfg").c_str(), &found);
    int files = 0, same = 0;
    std::string first;
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            const std::string path = std::string(folder) + "\\" + found.cFileName;
            std::FILE* f = std::fopen(path.c_str(), "rb");
            if (!f)
                continue;
            std::string data;
            char buffer[4096];
            size_t got = 0;
            while ((got = std::fread(buffer, 1, sizeof(buffer), f)) > 0)
                data.append(buffer, got);
            std::fclose(f);
            ++files;
            UndergrowthConfig stock;
            if (stock.parse(data, error) && stock.render() == data)
                ++same;
            else if (first.empty())
                first = found.cFileName;
        } while (FindNextFileA(h, &found));
        FindClose(h);
    }
    std::printf("[bf2_editor_checks] undergrowth samples: %d of %d written back byte for byte%s%s\n", same, files,
                first.empty() ? "" : "; first difference: ", first.c_str());
    CHECK(files > 0 && same == files);
}

// Save level as (Framework/LevelCopier): level names, path references retargeted (either slash, any case,
// word boundaries), and terraindata.raw's four header lines.
void checkLevelCopier()
{
    CHECK(validLevelName("My_Map2") && !validLevelName("my map") && !validLevelName("") && !validLevelName("a/b"));
    int count = 0;
    const std::string text = "run Levels/Strike_at_Karkand/Init.con\r\nlevels\\strike_at_karkand\\x.dds\r\n"
                             "MyLevels/Strike_at_Karkand/y\r\nLevels/Strike_at_Karkand2/z\r\nLEVELS/STRIKE_AT_KARKAND/w\r\n";
    const std::string expected = "run Levels/New_Map/Init.con\r\nlevels\\New_Map\\x.dds\r\nMyLevels/Strike_at_Karkand/y\r\n"
                                 "Levels/Strike_at_Karkand2/z\r\nLEVELS/New_Map/w\r\n";
    CHECK(retargetLevelPaths(text, "Strike_at_Karkand", "New_Map", count) == expected && count == 3);

    std::vector<unsigned char> raw(57, 0x11);
    const std::string lines = "Levels/Strike_at_Karkand/Colormaps/tx\nLevels/Strike_at_Karkand/Detailmaps/tx\n"
                              "Levels/Strike_at_Karkand/LowDetailmaps/tx\nLevels/Strike_at_Karkand/Lightmaps/tx\n";
    raw.insert(raw.end(), lines.begin(), lines.end());
    raw.push_back(0x42);
    raw.push_back(0x0A);                            // a '\n' byte in the body is left alone
    CHECK(retargetTerrainData(raw, "Strike_at_Karkand", "New_Map", count) && count == 4);
    const std::string want = "Levels/New_Map/Colormaps/tx\nLevels/New_Map/Detailmaps/tx\nLevels/New_Map/LowDetailmaps/tx\n"
                             "Levels/New_Map/Lightmaps/tx\n";
    CHECK(raw.size() == 57 + want.size() + 2 && std::string(raw.begin() + 57, raw.begin() + 57 + want.size()) == want
          && raw[0] == 0x11 && raw[raw.size() - 2] == 0x42);
    std::vector<unsigned char> tooShort(40, 0);
    CHECK(!retargetTerrainData(tooShort, "a", "b", count));
}

// Bundles and brushes (Framework/ObjectLibrary): the retail shapes read, written back in the retail
// writers' formats, the defects not copied, the refusals, the generator, and the folder round trip.
void checkObjectLibrary()
{
    // A brush in the shape of the shipped retail file: no precache block, an empty rem name, and rotation
    // ranges that repeat the position ranges (the retail writer's defect).
    const std::string retail =
        "rem *** ***\r\n"
        "LevelBrush.CreateBrush Bushes1\r\n"
        "LevelBrush.RandomPositionMin 10.000/0.000/5.000\r\n"
        "LevelBrush.RandomPositionMax 15.000/0.000/10.000\r\n"
        "LevelBrush.RandomRotationMin 10.000/0.000/5.000\r\n"
        "LevelBrush.RandomRotationMax 15.000/0.000/10.000\r\n"
        "LevelBrush.RandomScaleMin 0.000/0.000/0.000\r\n"
        "LevelBrush.RandomScaleMax 0.000/0.000/0.000\r\n"
        "LevelBrush.Spacing 50.000000\r\n"
        "LevelBrush.SnapToGround 1\r\n"
        "LevelBrush.AddItem bush_a\r\n"
        "LevelBrush.SetItemPosition -7.397/0.390/6.428\r\n"
        "LevelBrush.SetItemRotation 0.000/0.000/0.000\r\n"
        "LevelBrush.SetItemScale 0.000/0.000/0.000\r\n"
        "LevelBrush.AddItem bush_b\r\n"
        "LevelBrush.SetItemPosition 0.000/0.000/0.000\r\n"
        "LevelBrush.SetItemRotation 0.000/90.000/0.000\r\n"
        "LevelBrush.SetItemScale 0.000/0.000/0.000\r\n";
    LevelBrushFile brush;
    std::string error;
    CHECK(brush.parse(retail, error) && brush.name == "Bushes1" && brush.items.size() == 2);
    CHECK(brush.rotationRangesCopied && brush.spacing == 50.0f && brush.snapToGround && brush.preload.empty());
    CHECK(brush.items[0].templateName == "bush_a" && brush.items[1].templateName == "bush_b");     // the file's order
    CHECK(brush.items[0].position[0] == -7.397f && brush.items[1].rotation[1] == 90.0f);
    // Written back: the precache block the session gives it, the name in the rem line, the real rotation
    // ranges.
    brush.preload.push_back("run /objects/vegitation/bush_a/bush_a.con");
    brush.randomRotationMin[0] = -30.0f;
    brush.randomRotationMax[0] = 30.0f;
    brush.randomRotationMin[2] = brush.randomRotationMax[2] = 0.0f;
    const std::string expected =
        "console.allowMultipleFileLoad 0\r\n"
        "run /objects/vegitation/bush_a/bush_a.con\r\n"
        "console.allowMultipleFileLoad 1\r\n"
        "\r\n"
        "rem *** Bushes1 ***\r\n"
        "LevelBrush.CreateBrush Bushes1\r\n"
        "LevelBrush.RandomPositionMin 10.000/0.000/5.000\r\n"
        "LevelBrush.RandomPositionMax 15.000/0.000/10.000\r\n"
        "LevelBrush.RandomRotationMin -30.000/0.000/0.000\r\n"
        "LevelBrush.RandomRotationMax 30.000/0.000/0.000\r\n"
        "LevelBrush.RandomScaleMin 0.000/0.000/0.000\r\n"
        "LevelBrush.RandomScaleMax 0.000/0.000/0.000\r\n"
        "LevelBrush.Spacing 50.000000\r\n"
        "LevelBrush.SnapToGround 1\r\n"
        "LevelBrush.AddItem bush_a\r\n"
        "LevelBrush.SetItemPosition -7.397/0.390/6.428\r\n"
        "LevelBrush.SetItemRotation 0.000/0.000/0.000\r\n"
        "LevelBrush.SetItemScale 0.000/0.000/0.000\r\n"
        "LevelBrush.AddItem bush_b\r\n"
        "LevelBrush.SetItemPosition 0.000/0.000/0.000\r\n"
        "LevelBrush.SetItemRotation 0.000/90.000/0.000\r\n"
        "LevelBrush.SetItemScale 0.000/0.000/0.000\r\n";
    CHECK(brush.render() == expected);
    LevelBrushFile again;
    CHECK(again.parse(expected, error) && again.render() == expected && !again.rotationRangesCopied);
    // The item setters in the property grid's spelling, and the refusals.
    CHECK(again.parse("LevelBrush.CreateBrush x\r\nLevelBrush.AddItem t\r\nLevelBrushItem.ItemRotation 1/2/3\r\n", error)
          && again.items.size() == 1 && again.items[0].rotation[2] == 3.0f);
    CHECK(!again.parse("LevelBrush.Spacing 5\r\n", error));                                       // before CreateBrush
    CHECK(!again.parse("LevelBrush.CreateBrush x\r\nLevelBrush.SetItemPosition 1/2/3\r\n", error));  // before AddItem
    CHECK(!again.parse("LevelBrush.CreateBrush x\r\nLevelBrush.CreateBrush y\r\n", error));          // a second brush
    CHECK(!again.parse("LevelBrush.CreateBrush x\r\nLevelBrush.RandomPositionMin 1/2\r\n", error));  // not x/y/z
    CHECK(!again.parse("LevelBrush.CreateBrush x\r\nLevelBrush.Paint 1\r\n", error));                // unknown
    CHECK(!again.parse("rem nothing here\r\n", error));                                           // no brush

    // A bundle: the retail writer's shape and numbers.
    const std::string bundleText =
        "console.allowMultipleFileLoad 0\r\n"
        "run /objects/staticobjects/a/a.con\r\n"
        "console.allowMultipleFileLoad 1\r\n"
        "\r\n"
        "ObjectBundle.CreateRootObject crate 90.00/0.00/0.00 1\r\n"
        "ObjectBundle.CreateObject crate 4.50/0.00/-2.00 90.00/0.00/0.00 1\r\n"
        "ObjectBundle.CreateObject barrel -4.50/0.25/-2.00 0.00/0.00/0.00 3\r\n";
    ObjectBundleFile bundle;
    CHECK(bundle.parse(bundleText, error) && bundle.records.size() == 3 && bundle.preload.size() == 1);
    CHECK(bundle.records[0].templateName == "crate" && bundle.records[0].rotation[0] == 90.0f && bundle.records[0].layer == 1);
    CHECK(bundle.records[2].offset[1] == 0.25f && bundle.records[2].layer == 3 && bundle.render() == bundleText);
    ObjectBundleFile refused;
    CHECK(!refused.parse("ObjectBundle.CreateObject a 0/0/0 0/0/0 1\r\n", error));                  // before the root
    CHECK(!refused.parse("ObjectBundle.CreateRootObject a 0/0/0 1\r\nObjectBundle.CreateRootObject b 0/0/0 1\r\n", error));
    CHECK(!refused.parse("ObjectBundle.CreateRootObject a 0/0/0 256\r\n", error));                   // no such layer
    CHECK(!refused.parse("console.allowMultipleFileLoad 0\r\n", error));                             // no root

    // One generator, the same numbers from the same seed.
    LibraryRandom a(42), b(42);
    bool sameNumbers = true, inRange = true;
    for (int i = 0; i < 1000; ++i)
    {
        const float x = a.next();
        sameNumbers = sameNumbers && x == b.next();
        inRange = inRange && x >= 0.0f && x < 1.0f;
    }
    CHECK(sameNumbers && inRange);
    LibraryRandom c(1);
    CHECK(c.next() == static_cast<float>(48270.0 / 2147483646.0) && c.state() == 48271u);
    LibraryRandom zero(0);
    CHECK(zero.state() == 1u);
    CHECK(validLibraryName("My_Brush-2") && !validLibraryName("a.b") && !validLibraryName("") && !validLibraryName("a b"));

    // The folder: bundles saved and read back, brushes written through and read again.
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    char unique[64];
    std::snprintf(unique, sizeof(unique), "bf2editor_library_%lu", static_cast<unsigned long>(GetCurrentProcessId()));
    const std::string folder = std::string(temp) + unique + "/LevelEditor";
    ObjectLibrary library;
    library.setFolder(folder);
    CHECK(library.brushes().empty() && library.bundleNames().empty());
    CHECK(library.saveBundle("Camp", bundle, false, error) && library.bundleExists("Camp"));
    CHECK(!library.saveBundle("Camp", bundle, false, error) && library.saveBundle("Camp", bundle, true, error));
    ObjectBundleFile loaded;
    CHECK(library.loadBundle("Camp", loaded, error) && loaded.render() == bundle.render());
    CHECK(library.bundleNames().size() == 1 && library.bundleNames()[0] == "Camp");
    LevelBrushFile made;
    made.name = "Trees";
    made.items.push_back(LevelBrushItem());
    made.items[0].templateName = "tree_a";
    CHECK(library.addBrush(made, error) && !library.addBrush(made, error));
    CHECK(library.brush("trees") && library.brush("trees")->spacing == 10.0f);
    library.brush("Trees")->spacing = 4.5f;
    CHECK(library.writeBrush("Trees", error));
    library.reloadBrushes();
    CHECK(library.brushes().size() == 1 && library.brush("Trees") && library.brush("Trees")->spacing == 4.5f
          && library.brushProblems().empty());
    CHECK(library.deleteBrush("Trees", error) && !library.brush("Trees") && library.deleteBundle("Camp", error));
    RemoveDirectoryA((folder + "/LevelBrushes").c_str());
    RemoveDirectoryA((folder + "/ObjectBundle").c_str());
    RemoveDirectoryA(folder.c_str());
    RemoveDirectoryA((std::string(temp) + unique).c_str());
}

// Object template files (Framework/TemplateFile, WP7): every byte kept; blocks and their property lines;
// a value set in place; a new line placed as the census's approach A says - a component's inside its
// section, any other after the block's last line outside the sections; repeated properties by index; with
// BF2EDITOR_TEMPLATE_SAMPLES set to a folder of stock .con/.tweak files (extract_template_samples.py), each
// is written back byte for byte.
// TemplateFile::addProperty: one more line of a property that adds, after the block's last one of it.
void checkTemplateAddProperty()
{
    TemplateFile file;
    file.parse("ObjectTemplate.activeSafe SimpleObject crate\r\n"
               "ObjectTemplate.armor.maxHit 100\r\n"
               "ObjectTemplate.armor.addArmorEffect 0 e_a 0/0/0 0/0/0\r\n"
               "ObjectTemplate.hasCollisionPhysics 1\r\n");
    std::string error;
    CHECK(file.addProperty("crate", "armor.addArmorEffect", "0 e_b 0/0/0 0/0/0", error));
    CHECK(file.render() == "ObjectTemplate.activeSafe SimpleObject crate\r\n"
                           "ObjectTemplate.armor.maxHit 100\r\n"
                           "ObjectTemplate.armor.addArmorEffect 0 e_a 0/0/0 0/0/0\r\n"
                           "ObjectTemplate.armor.addArmorEffect 0 e_b 0/0/0 0/0/0\r\n"
                           "ObjectTemplate.hasCollisionPhysics 1\r\n");
    // A property with no line yet goes where a new set would (after the block's armor lines).
    CHECK(file.addProperty("crate", "armor.hpLostWhileUpSideDown", "5", error));
    CHECK(file.render().find("0 e_b 0/0/0 0/0/0\r\nObjectTemplate.armor.hpLostWhileUpSideDown 5\r\n") != std::string::npos);
    CHECK(!file.addProperty("nothing", "armor.maxHit", "1", error));
    CHECK(!file.addProperty("crate", "two words", "1", error));
}

void checkTemplateFile()
{
    const std::string text =
        "rem *** Generated with Bf2Editor.exe [created: 2005/4/26 9:32]\r\n"
        "GeometryTemplate.compressVertexData 1\r\n"
        "ObjectTemplate.activeSafe PlayerControlObject tank_a\r\n"
        "ObjectTemplate.creator XYZ-1:xyz\r\n"
        "ObjectTemplate.saveInSeparateFile 1\r\n"
        "rem ---BeginComp:Armor ---\r\n"
        "ObjectTemplate.createComponent Armor\r\n"
        "ObjectTemplate.armor.maxHitPoints 1000\r\n"
        "ObjectTemplate.armor.hitPoints  1000\r\n"
        "rem ---EndComp ---\r\n"
        "ObjectTemplate.geometry tank_a\r\n"
        "ObjectTemplate.addTemplate tank_a_turret\r\n"
        "ObjectTemplate.setPosition 0/1/0\r\n"
        "ObjectTemplate.addTemplate tank_a_engine\r\n"
        "\r\n"
        "ObjectTemplate.activeSafe RotationalBundle tank_a_turret\r\n"
        "ObjectTemplate.geometryPart 1\r\n"
        "\r\n";
    TemplateFile file;
    file.parse(text);
    CHECK(file.render() == text && !file.hasChanges() && file.blocks().size() == 2);
    CHECK(file.blocks()[0].name == "tank_a" && file.blocks()[0].className == "PlayerControlObject"
          && file.blocks()[0].first == 2 && file.blocks()[0].end == 15 && file.blocks()[1].end == 18);
    const std::vector<TemplatePropertyLine> lines = file.properties("TANK_A");
    CHECK(lines.size() == 9 && lines[0].property == "creator" && lines[4].property == "armor.hitPoints"
          && lines[4].value == "1000");
    std::string value, error;
    CHECK(file.value("tank_a", "ARMOR.MAXHITPOINTS", value) && value == "1000");
    // In place: the double blank before the value is kept.
    CHECK(file.setProperty("tank_a", "armor.hitPoints", "800", -1, error) && file.hasChanges());
    CHECK(file.line(8) == "ObjectTemplate.armor.hitPoints  800");
    // A new component line goes inside the Armor section; a new plain line after geometry... no: after the
    // block's last line outside the sections, which is the last addTemplate.
    CHECK(file.setProperty("tank_a", "armor.waterLevel", "-0.3", -1, error) && file.line(9) == "ObjectTemplate.armor.waterLevel -0.3"
          && file.line(10) == "rem ---EndComp ---");
    CHECK(file.setProperty("tank_a", "hasMobilePhysics", "1", -1, error) && file.line(15) == "ObjectTemplate.hasMobilePhysics 1"
          && file.line(16).empty());
    // Repeated properties need an index.
    CHECK(!file.setProperty("tank_a", "addTemplate", "x", -1, error));
    CHECK(file.setProperty("tank_a", "addTemplate", "tank_a_engine2", 1, error) && file.line(14) == "ObjectTemplate.addTemplate tank_a_engine2");
    CHECK(!file.setProperty("tank_a", "addTemplate", "x", 2, error) && !file.setProperty("nobody", "geometry", "x", -1, error));
    CHECK(file.removeProperty("tank_a", "creator", -1, error) && file.properties("tank_a")[0].property == "saveInSeparateFile");
    // A template the file has no lines of gets a block of its own at the end.
    file.appendBlock("SimpleObject", "tank_a_engine");
    CHECK(file.setProperty("tank_a_engine", "geometryPart", "3", -1, error) && file.blocks().size() == 3);
    const std::string tail = file.render().substr(file.render().size() - 90);
    CHECK(tail.find("\r\nObjectTemplate.activeSafe SimpleObject tank_a_engine\r\nObjectTemplate.geometryPart 3\r\n") != std::string::npos);
    // Line ends kept as they are: LF, mixed, and no final line end.
    TemplateFile lf;
    lf.parse("ObjectTemplate.create SimpleObject a\nObjectTemplate.geometry a\nrem x\r\nObjectTemplate.geometryPart 2");
    CHECK(lf.render() == "ObjectTemplate.create SimpleObject a\nObjectTemplate.geometry a\nrem x\r\nObjectTemplate.geometryPart 2");
    CHECK(lf.setProperty("a", "hasMobilePhysics", "1", -1, error)
          && lf.render() == "ObjectTemplate.create SimpleObject a\nObjectTemplate.geometry a\nrem x\r\nObjectTemplate.geometryPart 2\nObjectTemplate.hasMobilePhysics 1");
    CHECK(lf.setProperty("a", "hasMobilePhysics", "", -1, error) && lf.line(4) == "ObjectTemplate.hasMobilePhysics");
    // Children: a child added after the block's last child line, with its position; the first line of a child
    // taken out with the position and rotation lines that belong to it.
    TemplateFile pile;
    pile.parse("ObjectTemplate.create Bundle pile\r\nObjectTemplate.geometry pile\r\nObjectTemplate.addTemplate crate_a\r\n"
               "ObjectTemplate.setPosition 1/0/0\r\nObjectTemplate.setRotation 90/0/0\r\nObjectTemplate.addTemplate crate_b\r\n"
               "ObjectTemplate.hasMobilePhysics 0\r\n\r\nObjectTemplate.create SimpleObject crate_a\r\n");
    CHECK(pile.addChildLines("pile", "crate_c", "0/0/2", error) && pile.line(6) == "ObjectTemplate.addTemplate crate_c"
          && pile.line(7) == "ObjectTemplate.setPosition 0/0/2" && pile.line(8) == "ObjectTemplate.hasMobilePhysics 0");
    CHECK(pile.removeChildLines("pile", "CRATE_A", error) && pile.line(2) == "ObjectTemplate.addTemplate crate_b"
          && pile.line(3) == "ObjectTemplate.addTemplate crate_c" && pile.blocks().size() == 2);
    CHECK(!pile.removeChildLines("pile", "crate_z", error) && !pile.addChildLines("nobody", "x", "", error));
    TemplateFile lonely;
    lonely.parse("ObjectTemplate.create Bundle lonely\r\nObjectTemplate.geometry lonely\r\n");
    CHECK(lonely.addChildLines("lonely", "part", "", error)
          && lonely.render() == "ObjectTemplate.create Bundle lonely\r\nObjectTemplate.geometry lonely\r\nObjectTemplate.addTemplate part\r\n");
    // A child's position and rotation: rewritten in place (the blanks kept), or added right after its
    // addTemplate line - the rotation after the position; the children listed with their lines; one taken
    // out by its index.
    TemplateFile moving;
    moving.parse("ObjectTemplate.create Bundle moving\r\nObjectTemplate.addTemplate a\r\nObjectTemplate.setPosition  1/0/0\r\n"
                 "ObjectTemplate.addTemplate b\r\nObjectTemplate.addTemplate a\r\nObjectTemplate.setRotation 0/90/0\r\nrem end\r\n");
    const std::vector<TemplateChildLines> kids = moving.children("moving");
    CHECK(kids.size() == 3 && kids[0].child == "a" && kids[0].position == "1/0/0" && kids[0].rotationLine == -1
          && kids[0].end == 3 && kids[1].child == "b" && kids[1].positionLine == -1 && kids[2].rotation == "0/90/0"
          && kids[2].positionLine == -1 && kids[2].end == 6);
    CHECK(moving.setChildTransformAt("moving", 0, "2/0/0", "45/0/0", error)
          && moving.line(2) == "ObjectTemplate.setPosition  2/0/0" && moving.line(3) == "ObjectTemplate.setRotation 45/0/0");
    CHECK(moving.setChildTransformAt("moving", 1, "0/0/5", "", error) && moving.line(5) == "ObjectTemplate.setPosition 0/0/5"
          && moving.line(6) == "ObjectTemplate.addTemplate a");
    CHECK(moving.setChildTransformAt("moving", 2, "3/3/3", "0/0/0", error)
          && moving.line(7) == "ObjectTemplate.setPosition 3/3/3" && moving.line(8) == "ObjectTemplate.setRotation 0/0/0"
          && moving.line(9) == "rem end");
    CHECK(moving.removeChildAt("moving", 2, error) && moving.children("moving").size() == 2
          && moving.render() == "ObjectTemplate.create Bundle moving\r\nObjectTemplate.addTemplate a\r\n"
                                 "ObjectTemplate.setPosition  2/0/0\r\nObjectTemplate.setRotation 45/0/0\r\n"
                                 "ObjectTemplate.addTemplate b\r\nObjectTemplate.setPosition 0/0/5\r\nrem end\r\n");
    CHECK(!moving.setChildTransformAt("moving", 5, "0/0/0", "", error) && !moving.removeChildAt("moving", 2, error)
          && !moving.setChildTransformAt("moving", 0, "1 2", "", error));
    TemplateFile unended;
    unended.parse("ObjectTemplate.create Bundle tail\r\nObjectTemplate.addTemplate x");
    CHECK(unended.setChildTransformAt("tail", 0, "1/1/1", "", error)
          && unended.render() == "ObjectTemplate.create Bundle tail\r\nObjectTemplate.addTemplate x\r\nObjectTemplate.setPosition 1/1/1");
    // Component sections: a new one after the header lines (none yet) and after the last section; a
    // component's line set inside its section; one taken out with its lines kept, one replaced in place.
    TemplateFile componentFile;
    componentFile.parse("ObjectTemplate.activeSafe SimpleObject parts\r\nObjectTemplate.creator me\r\nObjectTemplate.setNetworkableInfo "
                "BasicInfo\r\nObjectTemplate.geometry parts\r\n\r\nObjectTemplate.activeSafe SimpleObject other\r\n");
    CHECK(componentFile.addComponentSection("parts", "Armor", error) && componentFile.line(3) == "rem ---BeginComp:Armor ---"
          && componentFile.line(4) == "ObjectTemplate.createComponent Armor" && componentFile.line(5) == "rem ---EndComp ---"
          && componentFile.line(6) == "ObjectTemplate.geometry parts");
    CHECK(componentFile.setProperty("parts", "armor.maxHitPoints", "50", -1, error) && componentFile.line(5) == "ObjectTemplate.armor.maxHitPoints 50"
          && componentFile.line(6) == "rem ---EndComp ---");
    CHECK(componentFile.addComponentSection("parts", "SingleFireComp", error) && componentFile.line(7) == "rem ---BeginComp:SingleFireComp ---"
          && componentFile.line(10) == "ObjectTemplate.geometry parts" && componentFile.componentSections("parts").size() == 2
          && componentFile.componentSections("parts")[1].end == 10 && componentFile.componentSections("other").empty());
    std::vector<std::string> sectionLines;
    CHECK(componentFile.replaceComponentSection("parts", "singlefirecomp", "MultiFireComp", sectionLines, error) && sectionLines.empty()
          && componentFile.line(7) == "rem ---BeginComp:MultiFireComp ---" && componentFile.line(8) == "ObjectTemplate.createComponent MultiFireComp");
    CHECK(componentFile.removeComponentSection("parts", "Armor", sectionLines, error) && sectionLines.size() == 1
          && sectionLines[0] == "ObjectTemplate.armor.maxHitPoints 50" && componentFile.line(3) == "rem ---BeginComp:MultiFireComp ---");
    CHECK(!componentFile.removeComponentSection("parts", "Armor", sectionLines, error) && !componentFile.addComponentSection("nobody", "Armor", error)
          && !componentFile.addComponentSection("parts", "two words", error));
    // A rename followed through a file: block openings and addTemplate lines, nothing else.
    TemplateFile renaming;
    renaming.parse("ObjectTemplate.create Bundle holder\r\nObjectTemplate.addTemplate  part \r\nObjectTemplate.projectileTemplate part\r\n"
                   "\r\nObjectTemplate.create SimpleObject part\r\nObjectTemplate.activeSafe SimpleObject PART\r\nObjectTemplate.active part\r\n");
    CHECK(renaming.renameTemplateLines("part", "piece")
          && renaming.render() == "ObjectTemplate.create Bundle holder\r\nObjectTemplate.addTemplate  piece \r\n"
                                  "ObjectTemplate.projectileTemplate part\r\n\r\nObjectTemplate.create SimpleObject piece\r\n"
                                  "ObjectTemplate.activeSafe SimpleObject piece\r\nObjectTemplate.active piece\r\n"
          && renaming.blocksOf("piece").size() == 3 && !renaming.renameTemplateLines("nothing", "x"));
    // A template's class changed in its openings, nowhere else.
    TemplateFile retyped;
    retyped.parse("ObjectTemplate.create  SimpleObject  thing\r\nObjectTemplate.geometry thing\r\n\r\n"
                  "ObjectTemplate.activeSafe SimpleObject thing\r\nObjectTemplate.active thing\r\nObjectTemplate.create Bundle other\r\n");
    CHECK(retyped.setBlockClass("thing", "Bundle")
          && retyped.render() == "ObjectTemplate.create  Bundle  thing\r\nObjectTemplate.geometry thing\r\n\r\n"
                                 "ObjectTemplate.activeSafe Bundle thing\r\nObjectTemplate.active thing\r\nObjectTemplate.create Bundle other\r\n"
          && retyped.blocks()[0].className == "Bundle" && !retyped.setBlockClass("thing", "Bundle")
          && !retyped.setBlockClass("nobody", "Bundle"));
    // A new child's block at the end of its parent's file.
    TemplateFile holder;
    holder.parse("ObjectTemplate.create Bundle holder\r\nObjectTemplate.geometry holder\r\n");
    holder.appendCreateBlock("SimpleObject", "holder_part", std::vector<std::string>(1, "createdInEditor 1"));
    CHECK(holder.render() == "ObjectTemplate.create Bundle holder\r\nObjectTemplate.geometry holder\r\n\r\n"
                             "ObjectTemplate.create SimpleObject holder_part\r\nObjectTemplate.createdInEditor 1\r\n"
          && holder.blocksOf("holder_part").size() == 1);
    // A clone: the source renamed in its openings, a part and a projectile the files create cloned and followed
    // (addTemplate, projectileTemplate), a child of a file of its own shared, the geometry and collision lines
    // left out and the geometry named as before, the include renamed.
    TemplateClone clone;
    const std::string cloneCon =
        "GeometryTemplate.create BundledMesh tank\r\nCollisionManager.createTemplate tank\r\n\r\n"
        "ObjectTemplate.create PlayerControlObject tank\r\nObjectTemplate.geometry tank\r\nObjectTemplate.collisionMesh tank\r\n"
        "ObjectTemplate.addTemplate tank_turret\r\nObjectTemplate.addTemplate shared_wheel\r\n\r\n"
        "ObjectTemplate.create RotationalBundle tank_turret\r\nObjectTemplate.geometryPart 1\r\n\r\ninclude tank.tweak\r\n";
    const std::string cloneTweak =
        "GeometryTemplate.compressVertexData 1\r\nObjectTemplate.activeSafe PlayerControlObject tank\r\n"
        "ObjectTemplate.setCollisionMesh tank\r\nObjectTemplate.create GenericProjectile tank_shell\r\n\r\n"
        "ObjectTemplate.activeSafe RotationalBundle tank_turret\r\nObjectTemplate.projectileTemplate  tank_shell\r\n";
    CHECK(cloneTemplateFiles(cloneCon, cloneTweak, "tank", "tank", "tank2", clone, error) && clone.droppedLines == 3
          && clone.renames.size() == 3 && clone.renames[1].to == "tank2_turret" && clone.renames[2].to == "tank2_shell");
    CHECK(clone.con == "\r\nObjectTemplate.create PlayerControlObject tank2\r\nObjectTemplate.geometry tank\r\n"
                       "ObjectTemplate.collisionMesh tank\r\nObjectTemplate.addTemplate tank2_turret\r\n"
                       "ObjectTemplate.addTemplate shared_wheel\r\n\r\nObjectTemplate.create RotationalBundle tank2_turret\r\n"
                       "ObjectTemplate.geometryPart 1\r\n\r\ninclude tank2.tweak\r\n");
    CHECK(clone.tweak == "ObjectTemplate.activeSafe PlayerControlObject tank2\r\nObjectTemplate.setCollisionMesh tank\r\n"
                         "ObjectTemplate.create GenericProjectile tank2_shell\r\n\r\n"
                         "ObjectTemplate.activeSafe RotationalBundle tank2_turret\r\nObjectTemplate.projectileTemplate  tank2_shell\r\n");
    CHECK(clonedChildName("wheel", "tank", "tank2") == "tank2_wheel" && clonedChildName("TANK_x", "tank", "t2") == "t2_x");
    // The sounds named after their owners follow them: the source's and a cloned child's (the engine makes
    // them; the files only activeSafe and add them), one the files create; a shared child's sound stays, and
    // an S_ template that is nobody's sound is cloned as a child.
    const std::string soundCon = "ObjectTemplate.create PlayerControlObject Tank\r\nObjectTemplate.addTemplate TANK_turret\r\n"
                                 "ObjectTemplate.addTemplate S_Tank_Ambient\r\nObjectTemplate.addTemplate S_shared_wheel_Rpm\r\n"
                                 "ObjectTemplate.create RotationalBundle TANK_turret\r\n";
    const std::string soundTweak = "ObjectTemplate.activeSafe Sound S_Tank_Ambient\r\nObjectTemplate.soundFilename a.wav\r\n"
                                   "ObjectTemplate.activeSafe RotationalBundle TANK_turret\r\n"
                                   "ObjectTemplate.addTemplate S_TANK_turret_RotationRpm\r\n"
                                   "ObjectTemplate.activeSafe Sound S_TANK_turret_RotationRpm\r\n"
                                   "ObjectTemplate.create Sound S_tank_turret_Fire\r\nObjectTemplate.create Sound S_odd\r\n";
    CHECK(cloneTemplateFiles(soundCon, soundTweak, "tank", "tank", "t2", clone, error) && clone.sounds == 3
          && clone.renames.size() == 6 && clone.renames[1].to == "t2_turret" && clone.renames[2].to == "S_t2_Ambient"
          && clone.renames[3].to == "S_t2_turret_RotationRpm" && clone.renames[4].to == "S_t2_turret_Fire"
          && clone.renames[5].to == "t2_S_odd");
    CHECK(clone.con == "ObjectTemplate.create PlayerControlObject t2\r\nObjectTemplate.addTemplate t2_turret\r\n"
                       "ObjectTemplate.addTemplate S_t2_Ambient\r\nObjectTemplate.addTemplate S_shared_wheel_Rpm\r\n"
                       "ObjectTemplate.create RotationalBundle t2_turret\r\n"
          && clone.tweak == "ObjectTemplate.activeSafe Sound S_t2_Ambient\r\nObjectTemplate.soundFilename a.wav\r\n"
                            "ObjectTemplate.activeSafe RotationalBundle t2_turret\r\n"
                            "ObjectTemplate.addTemplate S_t2_turret_RotationRpm\r\n"
                            "ObjectTemplate.activeSafe Sound S_t2_turret_RotationRpm\r\n"
                            "ObjectTemplate.create Sound S_t2_turret_Fire\r\nObjectTemplate.create Sound t2_S_odd\r\n");
    CHECK(!cloneTemplateFiles(cloneCon, cloneTweak, "tank", "tank", "TANK", clone, error)
          && !cloneTemplateFiles(cloneCon, cloneTweak, "tank", "tank", "a b", clone, error)
          && !cloneTemplateFiles(cloneCon, cloneTweak, "tank", "ghost", "g2", clone, error));
    TemplateFile unclosed;
    unclosed.parse("ObjectTemplate.activeSafe SimpleObject open\r\nrem ---BeginComp:Armor ---\r\nObjectTemplate.createComponent Armor\r\n");
    CHECK(unclosed.componentSections("open").size() == 1 && unclosed.componentSections("open")[0].end == 0
          && !unclosed.removeComponentSection("open", "Armor", sectionLines, error) && !unclosed.addComponentSection("open", "Ik", error));
    std::string property;
    CHECK(TemplateFile::splitPropertyLine("  ObjectTemplate.setPosition   1/2/3  ", property, value) && property == "setPosition" && value == "1/2/3");
    CHECK(!TemplateFile::splitPropertyLine("GeometryTemplate.compressVertexData 1", property, value));

    // The stock files, when a folder of them is given.
    char samples[MAX_PATH] = "";
    if (GetEnvironmentVariableA("BF2EDITOR_TEMPLATE_SAMPLES", samples, MAX_PATH) == 0)
        return;
    std::vector<std::string> folders(1, samples);
    int files = 0, same = 0;
    while (!folders.empty())
    {
        const std::string folder = folders.back();
        folders.pop_back();
        WIN32_FIND_DATAA found;
        HANDLE search = FindFirstFileA((folder + "\\*").c_str(), &found);
        if (search == INVALID_HANDLE_VALUE)
            continue;
        do
        {
            const std::string name = found.cFileName;
            if (name == "." || name == "..")
                continue;
            const std::string path = folder + "\\" + name;
            if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                folders.push_back(path);
                continue;
            }
            const size_t dot = name.rfind('.');
            const std::string extension = dot == std::string::npos ? std::string() : name.substr(dot);
            if (_stricmp(extension.c_str(), ".con") != 0 && _stricmp(extension.c_str(), ".tweak") != 0)
                continue;
            std::string data;
            if (!ObjectLibrary::readTextFile(path, data, error))
                continue;
            ++files;
            TemplateFile stock;
            stock.parse(data);
            if (stock.render() == data)
                ++same;
            else
                std::printf("template sample not kept byte for byte: %s\n", path.c_str());
        } while (FindNextFileA(search, &found));
        FindClose(search);
    }
    std::printf("[bf2_editor_checks] %d stock template files, %d written back byte for byte\n", files, same);
    CHECK(files > 0 && same == files);
}

// The object archives (Framework/ObjectArchives, WP7 step 4): the retail packager's filters, the zips an
// archive script mounts at Objects, and a loose folder packed into the two zips and extracted again, with a
// second extraction keeping what is there.
void checkObjectArchives()
{
    CHECK(objectArchiveTakes("Vehicles/Land/tank/tank.con", true) && objectArchiveTakes("Vehicles/Land/tank/tank.tweak", false));
    CHECK(!objectArchiveTakes("Vehicles/Land/tank/Meshes/tank.bundledMesh", true)
          && objectArchiveTakes("Vehicles/Land/tank/Meshes/tank.BundledMesh", false));
    CHECK(!objectArchiveTakes("a/b/texture.DDS", true) && objectArchiveTakes("a/b/texture.dds", false));
    CHECK(!objectArchiveTakes("a/b/tank.samples", false) && !objectArchiveTakes("a/source.psd", false)
          && objectArchiveTakes("a/b/tank.occ", true));
    const std::string script = "fileManager.mountArchive Objects_server.zip Objects\r\nfileManager.mountArchive Menu_server.zip Menu\r\n"
                               "fileManager.mountArchive Booster_server.zip objects\r\n\r\n";
    const std::vector<std::string> zips = archivesMountedAt(script, "Objects");
    CHECK(zips.size() == 2 && zips[0] == "Objects_server.zip" && zips[1] == "Booster_server.zip");

    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    char unique[64];
    std::snprintf(unique, sizeof(unique), "bf2editor_objects_%lu", static_cast<unsigned long>(GetCurrentProcessId()));
    const std::string root = std::string(temp) + unique;
    const std::string loose = root + "/Objects";
    std::string error;
    CHECK(ObjectLibrary::writeTextFile(loose + "/Vehicles/tank/tank.con", "ObjectTemplate.create SimpleObject tank\r\n", error));
    CHECK(ObjectLibrary::writeTextFile(loose + "/Vehicles/tank/tank.tweak", "ObjectTemplate.activeSafe SimpleObject tank\r\n", error));
    CHECK(ObjectLibrary::writeTextFile(loose + "/Vehicles/tank/Textures/tank_c.dds", std::string(3000, 'x'), error));
    CHECK(ObjectLibrary::writeTextFile(loose + "/Vehicles/tank/tank.samples", "SMP2", error));
    ObjectPackReport pack;
    CHECK(packObjectArchives(loose, root + "/mod", pack, error));
    CHECK(pack.clientFiles == 3 && pack.serverFiles == 2 && pack.left == 1 && pack.clientBytes > 0);
    unsigned long long content = 0;
    CHECK(archiveContentSize(root + "/mod/Objects_client.zip", content, error) && content == 3000 + 41 + 45);
    ExtractReport first, again;
    CHECK(extractArchive(root + "/mod/Objects_client.zip", root + "/out", first, error) && first.files == 3 && first.kept == 0);
    std::string text;
    CHECK(ObjectLibrary::readTextFile(root + "/out/Vehicles/tank/tank.tweak", text, error)
          && text == "ObjectTemplate.activeSafe SimpleObject tank\r\n");
    CHECK(ObjectLibrary::writeTextFile(root + "/out/Vehicles/tank/tank.tweak", "edited", error));
    CHECK(extractArchive(root + "/mod/Objects_client.zip", root + "/out", again, error) && again.files == 0 && again.kept == 3);
    CHECK(ObjectLibrary::readTextFile(root + "/out/Vehicles/tank/tank.tweak", text, error) && text == "edited");
    CHECK(!extractArchive(root + "/mod/none.zip", root + "/out", again, error));
    // Clean up the temporary tree.
    const char* const files[] = { "/Objects/Vehicles/tank/tank.con", "/Objects/Vehicles/tank/tank.tweak",
                                  "/Objects/Vehicles/tank/Textures/tank_c.dds", "/Objects/Vehicles/tank/tank.samples",
                                  "/out/Vehicles/tank/tank.con", "/out/Vehicles/tank/tank.tweak",
                                  "/out/Vehicles/tank/Textures/tank_c.dds", "/mod/Objects_client.zip", "/mod/Objects_server.zip" };
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i)
        DeleteFileA((root + files[i]).c_str());
    const char* const folders[] = { "/Objects/Vehicles/tank/Textures", "/Objects/Vehicles/tank", "/Objects/Vehicles", "/Objects",
                                    "/out/Vehicles/tank/Textures", "/out/Vehicles/tank", "/out/Vehicles", "/out", "/mod", "" };
    for (size_t i = 0; i < sizeof(folders) / sizeof(folders[0]); ++i)
        RemoveDirectoryA((root + folders[i]).c_str());
}

// Checks mod files, own-before-base archive ordering, menu copies, and discoverable Mod.desc output.
void checkModScaffold()
{
    std::vector<std::string> bases(1, "bf2");
    const std::vector<std::pair<std::string, std::string> > files = modScaffoldFiles("MyMod", bases);
    CHECK(files.size() == 7 && files[0].first == "ClientArchives.con" && files[4].first == "MyMod.mew");
    CHECK(files[0].second == "fileManager.mountArchive Objects_client.zip Objects\r\n"
                             "fileManager.mountArchive Common_client.zip Common\r\n"
                             "fileManager.mountArchive Menu_client.zip Menu\r\n"
                             "fileManager.mountArchive Fonts_client.zip Fonts\r\n\r\n"
                             "fileManager.mountArchive mods/bf2/Objects_client.zip Objects\r\n"
                             "fileManager.mountArchive mods/bf2/Common_client.zip Common\r\n"
                             "fileManager.mountArchive mods/bf2/Menu_client.zip Menu\r\n"
                             "fileManager.mountArchive mods/bf2/Fonts_client.zip Fonts\r\n"
                             "fileManager.mountArchive mods/bf2/Shaders_client.zip Shaders\r\n");
    CHECK(files[1].second.find("\r\n\r\nfileManager.mountArchive mods/bf2/Objects_server.zip Objects\r\n") != std::string::npos
          && files[1].second.find("Shaders") == std::string::npos);
    static const unsigned char bf2Mew[] = { 2, 0, 0, 0, '.', '/', 0, 0, 0, 0, 6, 0, 0, 0, 'I', 'n', 'g', 'a', 'm', 'e',
                                            0, 0, 0, 0, 6, 0, 0, 0, 'I', 'n', 'g', 'a', 'm', 'e' };
    CHECK(files[4].second == std::string(reinterpret_cast<const char*>(bf2Mew), sizeof(bf2Mew)));
    CHECK(files[5].first == "Ingame.mep" && files[5].second.size() == 4 * 3 + 16 + 20 + 28
          && files[5].second.compare(0, 4, std::string("\x10\0\0\0", 4)) == 0);
    CHECK(files[6].first == "Mod.desc" && files[6].second.find("<title> MyMod </title>") != std::string::npos);
    CHECK(files[3].second.find("bf2Engine.playMovie mods\\bf2\\Movies\\EA.bik 1\r\n") != std::string::npos);
    CHECK(validModName("my_mod-2") && !validModName("my mod") && !validModName("a/b") && !validModName(""));

    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    char unique[64];
    std::snprintf(unique, sizeof(unique), "bf2editor_mods_%lu", static_cast<unsigned long>(GetCurrentProcessId()));
    const std::string mods = std::string(temp) + unique;
    std::string error;
    CHECK(createMod(mods, "MyMod", bases, error) && !createMod(mods, "MyMod", bases, error) && !createMod(mods, "bad name", bases, error));
    const std::vector<std::string> listed = listMods(mods);
    CHECK(listed.size() == 1 && listed[0] == "MyMod");
    for (size_t i = 0; i < files.size(); ++i)
        DeleteFileA((mods + "/MyMod/" + files[i].first).c_str());
    RemoveDirectoryA((mods + "/MyMod").c_str());
    RemoveDirectoryA(mods.c_str());
}

// The undergrowth density map (Framework/UndergrowthMap): square maps only; the renderer's cell mapping
// (column floor(x / scale) + width / 2, rows running with z); a disc painted over the cells whose centres
// it covers; the changed rectangle taken since a stroke began, and put back both ways.
void checkUndergrowthMap()
{
    UndergrowthMap map;
    std::string error;
    CHECK(!map.load(std::string(24, '\0'), error) && map.load(std::string(25, '\1'), error) && map.size() == 5);
    int column = 0, row = 0;
    CHECK(map.cellAt(0.0f, 0.0f, 2.0f, column, row) && column == 2 && row == 2);
    CHECK(map.cellAt(-2.5f, 3.9f, 2.0f, column, row) && column == 0 && row == 3);
    CHECK(!map.cellAt(-5.0f, 0.0f, 2.0f, column, row) && !map.cellAt(0.0f, 6.0f, 2.0f, column, row));
    const std::string before = map.bytes();
    CHECK(map.paintDisc(0.0f, 0.0f, 1.5f, 1.0f, 7) && !map.paintDisc(0.0f, 0.0f, 1.5f, 1.0f, 7));
    CHECK(map.at(1, 1) == 7 && map.at(2, 2) == 7 && map.at(1, 2) == 7 && map.at(2, 1) == 7 && map.at(0, 1) == 1
          && map.at(3, 3) == 1);
    const std::vector<int> counts = map.counts();
    CHECK(counts[7] == 4 && counts[1] == 21);
    const UndergrowthChange change = map.changeSince(before);
    CHECK(change.column == 1 && change.row == 1 && change.columns == 2 && change.rows == 2 && change.before.size() == 4
          && change.after[0] == 7 && change.before[3] == 1);
    map.apply(change, false);
    CHECK(map.bytes() == before);
    map.apply(change, true);
    CHECK(map.at(2, 2) == 7 && map.changeSince(map.bytes()).empty());
}

// The object wizard's rules: the recorder's capture with its
// whole-line, case-insensitive, untrimmed terminator; reuse by title (case-sensitive) and append-only blocks;
// the load-time commands; the refusals before the first create and on a wrong argument count; substitution.
void checkWizardRules()
{
    WizardRules wizard;
    std::string command, message;
    std::vector<std::string> args;
    CHECK(wizard.feed("Wizard.warning x", command, args, message) == WizardRules::Refused);
    CHECK(wizard.feed("Wizard.create \"Check Has Camera\"", command, args, message) == WizardRules::Handled
          && wizard.current() == 0 && wizard.rules()[0].title == "Check Has Camera");
    CHECK(wizard.feed("Wizard.beginAnalyzer", command, args, message) == WizardRules::Handled && wizard.recording());
    CHECK(wizard.feed("Wizard.populate IS PlayerControlObject", command, args, message) == WizardRules::Captured);
    CHECK(wizard.feed(" Wizard.endAnalyzer", command, args, message) == WizardRules::Captured);
    CHECK(wizard.feed("WIZARD.ENDANALYZER", command, args, message) == WizardRules::Handled && !wizard.recording()
          && wizard.rules()[0].analyzer.size() == 2 && wizard.rules()[0].analyzer[1] == " Wizard.endAnalyzer");
    CHECK(wizard.feed("Wizard.warning \"#OBJECT has no Camera\"", command, args, message) == WizardRules::Handled
          && wizard.rules()[0].warning == "#OBJECT has no Camera");
    CHECK(wizard.feed("Wizard.warning", command, args, message) == WizardRules::Handled && message == "#OBJECT has no Camera");
    CHECK(wizard.feed("Wizard.type ErrorCheck", command, args, message) == WizardRules::Handled
          && wizard.feed("Wizard.category General", command, args, message) == WizardRules::Handled
          && wizard.rules()[0].type == "ErrorCheck" && wizard.rules()[0].category == "General");
    CHECK(wizard.feed("Wizard.affectAll 1", command, args, message) == WizardRules::Handled && wizard.rules()[0].affectAll);
    CHECK(wizard.feed("Wizard.modifyObjectList \"Pick some\" 0", command, args, message) == WizardRules::Handled
          && wizard.rules()[0].listOn && !wizard.rules()[0].listDefault && wizard.rules()[0].listTitle == "Pick some");
    CHECK(wizard.feed("Wizard.CheckValue maxSpeed EQUAL 0/0/0", command, args, message) == WizardRules::Command
          && command == "checkvalue" && args.size() == 3 && args[2] == "0/0/0");
    CHECK(wizard.feed("Wizard.CheckValue networkableInfo DIFFER \"\"", command, args, message) == WizardRules::Command
          && args.size() == 3 && args[2].empty());
    CHECK(wizard.feed("Wizard.nonsense", command, args, message) == WizardRules::Refused
          && wizard.feed("Wizard.populate IS", command, args, message) == WizardRules::Refused);
    CHECK(wizard.feed("ObjectTemplate.active x", command, args, message) == WizardRules::Console
          && wizard.feed("", command, args, message) == WizardRules::Empty);
    CHECK(wizard.feed("Wizard.create \"check has camera\"", command, args, message) == WizardRules::Handled
          && wizard.rules().size() == 2 && wizard.current() == 1);
    CHECK(wizard.feed("Wizard.create \"Check Has Camera\"", command, args, message) == WizardRules::Handled
          && wizard.rules().size() == 2 && wizard.current() == 0);
    CHECK(wizard.feed("Wizard.beginResolution", command, args, message) == WizardRules::Handled
          && wizard.feed("Wizard.create \"swallowed\"", command, args, message) == WizardRules::Captured
          && wizard.feed("Wizard.endResolution", command, args, message) == WizardRules::Handled
          && wizard.rules()[0].resolution.size() == 1 && wizard.rules().size() == 2);
    CHECK(wizard.feed("Wizard.beginAnalyzer", command, args, message) == WizardRules::Handled
          && wizard.feed("Wizard.populate IS Spring", command, args, message) == WizardRules::Captured
          && wizard.feed("Wizard.endAnalyzer", command, args, message) == WizardRules::Handled
          && wizard.rules()[0].analyzer.size() == 3);
    std::vector<std::string> lines = wizardFileLines("a\r\n\r\nb\nc");
    CHECK(lines.size() == 3 && lines[0] == "a" && lines[1] == "b" && lines[2] == "c");
    std::map<int, float> floats;
    floats[1] = 2.5f;
    CHECK(wizardSubstitute("x #FLOAT1 #FLOAT0 #STRING #OBJECT #OBJECT", floats, "s", "obj") == "x 2.500000 -1 s obj #OBJECT");
    CHECK(wizardFilterOperator("IS") == 1 && wizardFilterOperator("is") == 0 && wizardFilterOperator("HASCHILD") == 3
          && wizardCompareOperator("DIFFER") == 2 && wizardCompareOperator("") == 0);
    args = wizardArguments("a \"b c\" \"\" d");
    CHECK(args.size() == 4 && args[1] == "b c" && args[2].empty() && args[3] == "d");
}

// The material editor's files: every
// byte kept; values set in place; new lines, materials and cells where the engine's writer puts them; defaults
// and an unset damage modifier taken out; the writer's rules as a lint. With BF2EDITOR_MATERIAL_SAMPLES set to
// a folder holding the stock materialManagerDefine.con and materialManagerSettings.con
// (extract_material_samples.py), both are written back byte for byte and break no rule.
namespace
{

std::vector<std::string> materialLines(const std::string& text)
{
    std::vector<std::string> out;
    size_t start = 0;
    while (start < text.size())
    {
        size_t end = text.find("\r\n", start);
        if (end == std::string::npos)
            end = text.size();
        out.push_back(text.substr(start, end - start));
        start = end + 2;
    }
    return out;
}

} // namespace

// The retail command catalogue's table (HelpDocuments.h) and the Console panel's completion.
std::string utf16Cell(const char* text)
{
    std::string out;
    for (const char* p = text; *p; ++p)
    {
        out += *p;
        out += '\0';
    }
    out += '\0';
    out += '\0';
    return out;
}

void checkHelpAndCompletion()
{
    // Three rows of two columns (the headings, then two commands), then the 8-byte trailer.
    std::string bytes("\x03\0\0\0\x02\0\0\0", 8);
    bytes += utf16Cell("LANGUAGE") + utf16Cell("Description") + utf16Cell("Global.OnNew") + utf16Cell("New in plugin")
             + utf16Cell("Camera.Reset") + utf16Cell("");
    bytes += std::string("\x01\x02\x03\x04\x05\x06\x07\x08", 8);
    CommandCatalogue catalogue;
    std::string error;
    CHECK(catalogue.parse(bytes, error));
    CHECK(catalogue.size() == 2);
    CHECK(catalogue.name(0) == "Global.OnNew" && catalogue.description(0) == "New in plugin");
    CHECK(catalogue.describe("global.onnew") && *catalogue.describe("global.onnew") == "New in plugin");
    CHECK(catalogue.describe("Camera.Reset") && catalogue.describe("Camera.Reset")->empty());
    CHECK(!catalogue.describe("Camera.Nothing"));
    // A non-ASCII character comes out as UTF-8 (U+00E9 as C3 A9), a surrogate pair as one four-byte character.
    std::string accented("\x02\0\0\0\x02\0\0\0", 8);
    accented += utf16Cell("A") + utf16Cell("B") + utf16Cell("X.y");
    accented += std::string("\x63\0\xE9\0\x3D\xD8\x00\xDE\0\0", 10);
    CHECK(catalogue.parse(accented, error) && catalogue.size() == 1);
    CHECK(catalogue.description(0) == "c\xC3\xA9\xF0\x9F\x98\x80");
    // Truncated inside the table, or counts that are no catalogue: refused.
    CHECK(!catalogue.parse(bytes.substr(0, 30), error) && catalogue.size() == 0);
    CHECK(!catalogue.parse(std::string("\0\0\0\0\x02\0\0\0", 8), error));
    // The documents: a missing folder lists none, and a lookup finds by label or file name.
    CHECK(helpDocuments("no/such/help/folder").empty());
    std::vector<HelpDocument> documents(2);
    documents[0].label = "User guide";
    documents[0].path = "bf2editor/Help/UserGuide.doc";
    documents[1].label = "Workshop: LevelWorkshop";
    documents[1].path = "bf2editor/Help/Workshop/LevelWorkshop.doc";
    CHECK(findHelpDocument(documents, "user GUIDE") == &documents[0]);
    CHECK(findHelpDocument(documents, "levelworkshop.doc") == &documents[1]);
    CHECK(!findHelpDocument(documents, "Tutorial: none"));

    ConsoleCompletion completion;
    std::vector<std::string> names;
    names.push_back("EditorSelection.translate");
    names.push_back("EditorSelection.rotate");
    names.push_back("EditorSelection.select");
    names.push_back("EditorSelection.selectTemplate");
    names.push_back("EditorSettings.save");
    names.push_back("editorselection.TRANSLATE");    // the same name in other case: kept once
    names.push_back("Camera.position");
    names.push_back("notaname");
    completion.setNames(names);
    CHECK(completion.size() == 6);
    ConsoleCompletion::Result r = completion.complete("EditorSel", 10);
    CHECK(r.completed && r.text == "EditorSelection." && r.candidates.size() == 1);
    r = completion.complete("Editor", 10);
    CHECK(r.completed && r.text == "EditorSe" && r.candidates.size() == 2);
    r = completion.complete("editorselection.tr", 10);
    CHECK(r.completed && r.text == "EditorSelection.translate ");
    r = completion.complete("EditorSelection.sel", 10);
    CHECK(r.completed && r.text == "EditorSelection.select" && r.candidates.size() == 2);
    r = completion.complete("EditorSelection.select", 10);
    CHECK(!r.completed && r.candidates.size() == 2);
    r = completion.complete("EditorSelection.translate 0/1", 10);
    CHECK(!r.completed && r.candidates.empty());
    r = completion.complete("Nothing", 10);
    CHECK(!r.completed && r.text == "Nothing");
    r = completion.complete("  cam", 10);
    CHECK(r.completed && r.text == "  Camera.");
    CHECK(completion.matches("EditorSelection.", 2).size() == 2);
    CHECK(completion.matches("EditorSelection.translate 0/1/0", 8).size() == 1);
    CHECK(completion.matches("EditorSelection.trans 0/1/0", 8).empty());
}

// An animation system (AnimationSystemFile.h): the cursor idiom read, values set in place, new lines after their
// object, bundle items with their start times, objects added and taken out - every other byte kept. With
// BF2EDITOR_ANIMATION_SAMPLES set to the folder extract_animation_samples.py fills, every stock system comes back
// byte for byte and holds objects.
namespace
{

void walkIncFiles(const std::string& folder, std::vector<std::string>& out)
{
    WIN32_FIND_DATAA found;
    HANDLE h = FindFirstFileA((folder + "\\*").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do
    {
        const std::string name = found.cFileName;
        if (name == "." || name == "..")
            continue;
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            walkIncFiles(folder + "\\" + name, out);
        else if (name.size() > 4 && _stricmp(name.c_str() + name.size() - 4, ".inc") == 0)
            out.push_back(folder + "\\" + name);
    } while (FindNextFileA(h, &found));
    FindClose(h);
}

} // namespace

void checkAnimationSystemFile()
{
    const std::string stock =
        "animationSystem.cameraSpring.use 1\r\n"
        "animationSystem.cameraSpring.look 1/1\r\n"
        "\r\n"
        "rem *** Animations ***\r\n"
        "animationSystem.createAnimation Objects/W/a_fire.baf\r\n"
        "animationManager.looping 0\r\n"
        "animationManager.length 0.333333\r\n"
        "animationSystem.createAnimation Objects/W/a_idle.baf\r\n"
        "\r\n"
        "rem *** Bundles ***\r\n"
        "animationSystem.createBundle fire\r\n"
        "animationBundle.isLooping 0\r\n"
        "animationBundle.fadeOutTime 0\r\n"
        "animationBundle.addAnimation Objects/W/a_fire.baf\r\n"
        "animationBundle.addEvent sound:0_first\r\n"
        "animationBundle.setEventStartTime sound:0_first 0.2\r\n"
        "\r\n"
        "animationSystem.createBundle idle\r\n"
        "animationBundle.addAnimation Objects/W/a_idle.baf\r\n"
        "\r\n"
        "rem *** Triggers ***\r\n"
        "animationSystem.createTrigger Trigger still\r\n"
        "animationTrigger.addBundle idle\r\n"
        "animationSystem.createTrigger MessageTrigger shoot\r\n"
        "animationTrigger.message fire\r\n"
        "animationTrigger.addBundle fire\r\n";
    AnimationSystemFile file;
    file.parse(stock);
    CHECK(file.render() == stock);
    CHECK(file.objects().size() == 6);
    const AnimationObject* fire = file.find(AnimationKind::Bundle, "FIRE");
    CHECK(fire && fire->name == "fire" && fire->createLine == 10 && fire->lastLine == 15);
    const AnimationObject* shoot = file.find(AnimationKind::Trigger, "shoot");
    CHECK(shoot && shoot->type == "MessageTrigger");
    CHECK(file.properties(AnimationKind::Animation, "Objects/W/a_fire.baf").size() == 2);
    CHECK(file.systemLines().size() == 2 && file.systemLines()[1].property == "cameraSpring.look");
    std::vector<BundleItem> events = file.bundleEvents("fire");
    CHECK(events.size() == 1 && events[0].name == "sound:0_first" && events[0].startSet && events[0].start == 0.2f);
    std::vector<BundleItem> animations = file.bundleAnimations("fire");
    CHECK(animations.size() == 1 && !animations[0].startSet);
    std::string error;
    // In place, then a new setting after the bundle's settings (the writer's order: settings, animations, events).
    CHECK(file.setProperty(AnimationKind::Bundle, "fire", "fadeOutTime", "0.25", -1, error));
    CHECK(file.render().find("animationBundle.fadeOutTime 0.25\r\n") != std::string::npos);
    CHECK(file.setProperty(AnimationKind::Bundle, "fire", "abruptPlayback", "1", -1, error));
    CHECK(file.render().find("animationBundle.fadeOutTime 0.25\r\nanimationBundle.abruptPlayback 1\r\n"
                             "animationBundle.addAnimation Objects/W/a_fire.baf\r\n") != std::string::npos);
    // A new animation after the bundle's animations and before its events.
    CHECK(file.addLine(AnimationKind::Bundle, "fire", "addAnimation", "Objects/W/a_more.baf", error));
    CHECK(file.render().find("animationBundle.addAnimation Objects/W/a_fire.baf\r\nanimationBundle.addAnimation Objects/W/a_more.baf\r\n"
                             "animationBundle.addEvent sound:0_first\r\n") != std::string::npos);
    CHECK(file.removeBundleItem("fire", "Objects/W/a_more.baf", false, error) && file.bundleAnimations("fire").size() == 1);
    // A start time: after its addAnimation line; then rewritten in place.
    CHECK(file.setAnimationStart("fire", "Objects/W/a_fire.baf", 0.5f, error));
    CHECK(file.render().find("animationBundle.addAnimation Objects/W/a_fire.baf\r\n"
                             "animationBundle.setAnimationStartTime Objects/W/a_fire.baf 0.5\r\n") != std::string::npos);
    CHECK(file.setAnimationStart("fire", "Objects/W/a_fire.baf", 0.75f, error));
    CHECK(file.bundleAnimations("fire")[0].start == 0.75f);
    CHECK(!file.setAnimationStart("fire", "Objects/W/none.baf", 1.0f, error));
    CHECK(file.setEventStart("fire", "sound:0_first", 0.3f, error) && file.bundleEvents("fire")[0].start == 0.3f);
    // A length: never 0 (the engine divides by it); after the start-time line; then in place.
    CHECK(!file.setAnimationLength("fire", "Objects/W/a_fire.baf", 0.0f, error));
    CHECK(file.setAnimationLength("fire", "Objects/W/a_fire.baf", 1.5f, error));
    CHECK(file.render().find("animationBundle.setAnimationStartTime Objects/W/a_fire.baf 0.75\r\n"
                             "animationBundle.setAnimationLength Objects/W/a_fire.baf 1.5\r\n") != std::string::npos);
    CHECK(file.setAnimationLength("fire", "Objects/W/a_fire.baf", 2.0f, error) && file.bundleAnimations("fire")[0].lengthSet
          && file.bundleAnimations("fire")[0].length == 2.0f);
    // The animation taken out with its start-time and length lines; an event added.
    CHECK(file.removeBundleItem("fire", "Objects/W/a_fire.baf", false, error));
    CHECK(file.bundleAnimations("fire").empty() && file.render().find("setAnimationStartTime") == std::string::npos
          && file.render().find("setAnimationLength") == std::string::npos);
    CHECK(file.addLine(AnimationKind::Bundle, "fire", "addEvent", "effect:0", error) && file.bundleEvents("fire").size() == 2);
    // A new bundle after the last bundle, a blank line before it; taken out again with that blank line.
    const std::string before = file.render();
    CHECK(file.addObject(AnimationKind::Bundle, "", "reload", error));
    CHECK(file.render().find("animationBundle.addAnimation Objects/W/a_idle.baf\r\n\r\nanimationSystem.createBundle reload\r\n")
          != std::string::npos);
    CHECK(!file.addObject(AnimationKind::Bundle, "", "reload", error));
    CHECK(file.removeObject(AnimationKind::Bundle, "reload", error) && file.render() == before);
    // Name order, as the writer saves: a bundle that sorts first goes above the first bundle (a blank line after
    // it); an animation between two animations with no blank lines; both taken out again.
    CHECK(file.addObject(AnimationKind::Bundle, "", "aim", error));
    CHECK(file.render().find("rem *** Bundles ***\r\nanimationSystem.createBundle aim\r\n\r\nanimationSystem.createBundle fire\r\n")
          != std::string::npos);
    CHECK(file.addObject(AnimationKind::Animation, "", "Objects/W/a_fire2.baf", error));
    CHECK(file.render().find("animationManager.length 0.333333\r\nanimationSystem.createAnimation Objects/W/a_fire2.baf\r\n"
                             "animationSystem.createAnimation Objects/W/a_idle.baf\r\n") != std::string::npos);
    CHECK(file.removeObject(AnimationKind::Animation, "Objects/W/a_fire2.baf", error)
          && file.removeObject(AnimationKind::Bundle, "aim", error) && file.render() == before);
    // Value holders follow one another in name order.
    AnimationSystemFile holders;
    holders.parse("AnimationSystem.createValueHolder a_move\r\nAnimationValueHolder.values 0.25 4.2 10\r\n"
                  "AnimationSystem.createValueHolder c_turn\r\nAnimationValueHolder.values -1 -3 -10\r\n");
    CHECK(holders.addObject(AnimationKind::ValueHolder, "", "b_jump", error)
          && holders.setProperty(AnimationKind::ValueHolder, "b_jump", "values", "0 1 2", -1, error));
    CHECK(holders.render() == "AnimationSystem.createValueHolder a_move\r\nAnimationValueHolder.values 0.25 4.2 10\r\n"
                              "AnimationSystem.createValueHolder b_jump\r\nAnimationValueHolder.values 0 1 2\r\n"
                              "AnimationSystem.createValueHolder c_turn\r\nAnimationValueHolder.values -1 -3 -10\r\n");
    CHECK(!file.addObject(AnimationKind::Trigger, "", "nameless", error));
    // The trigger tree: roots, children first in the writer's order, a reference the engine finds only when its
    // object was created above it, a new child kept above the block that names it, the references swept.
    CHECK(file.triggerRoots().size() == 2 && file.triggerRoots()[0] == "still" && file.triggerRoots()[1] == "shoot");
    CHECK(file.addLine(AnimationKind::Trigger, "shoot", "addChild", "still", error));
    CHECK(file.render().find("animationSystem.createTrigger MessageTrigger shoot\r\nanimationTrigger.addChild still\r\n"
                             "animationTrigger.message fire\r\n") != std::string::npos);
    CHECK(file.triggerRoots().size() == 1 && file.triggerChildren("shoot").size() == 1 && file.triggerBundles("shoot").size() == 1);
    CHECK(file.createdBefore(AnimationKind::Trigger, "still", file.find(AnimationKind::Trigger, "shoot")->createLine)
          && !file.createdBefore(AnimationKind::Trigger, "shoot", file.find(AnimationKind::Trigger, "still")->createLine));
    const std::string withChild = file.render();
    CHECK(file.addObject(AnimationKind::Trigger, "IdleTrigger", "wait", error, file.find(AnimationKind::Trigger, "shoot")->createLine));
    CHECK(file.render().find("animationSystem.createTrigger IdleTrigger wait\r\n\r\nanimationSystem.createTrigger MessageTrigger shoot\r\n")
          != std::string::npos);
    CHECK(file.addLine(AnimationKind::Trigger, "shoot", "addChild", "wait", error) && file.triggerSubtree("shoot").size() == 3);
    CHECK(file.removeReferences(AnimationKind::Trigger, "addChild", "WAIT") == 1 && file.removeObject(AnimationKind::Trigger, "wait", error)
          && file.render() == withChild);
    CHECK(file.removeReferences(AnimationKind::Trigger, "addChild", "still") == 1 && file.triggerRoots().size() == 2);
    // The system's own line in place.
    CHECK(file.setSystemProperty("cameraSpring.look", "2/2", error) && file.render().find("animationSystem.cameraSpring.look 2/2\r\n") == 36);
    CHECK(file.setSystemProperty("cameraSpring.fire", "0/0/150", error) && file.systemLines().size() == 3
          && file.removeSystemProperty("cameraSpring.fire", error) && file.systemLines().size() == 2);
    AnimationKind parsed = AnimationKind::Animation;
    CHECK(AnimationSystemFile::kindFromName("Bundle", parsed) && parsed == AnimationKind::Bundle
          && AnimationSystemFile::kindFromName("valueholder", parsed) && parsed == AnimationKind::ValueHolder
          && !AnimationSystemFile::kindFromName("skeleton", parsed));

    // The stock systems, when a folder of them is given.
    char samples[MAX_PATH] = "";
    if (GetEnvironmentVariableA("BF2EDITOR_ANIMATION_SAMPLES", samples, MAX_PATH) == 0)
        return;
    std::vector<std::string> files;
    walkIncFiles(samples, files);
    CHECK(files.size() > 170);
    int identical = 0, withObjects = 0;
    for (size_t i = 0; i < files.size(); ++i)
    {
        std::string text;
        if (!ObjectLibrary::readTextFile(files[i], text, error))
            continue;
        AnimationSystemFile one;
        one.parse(text);
        if (one.render() == text)
            ++identical;
        if (!one.objects().empty())
            ++withObjects;
    }
    CHECK(identical == static_cast<int>(files.size()));
    CHECK(withObjects >= static_cast<int>(files.size()) - 2);
}

// The retail layer model of gameplay objects (GameplayLayers.h): the writer's layout parsed and rendered back, the
// layers' file, and stock levels imported from their game-mode files - every bucket kept regenerated byte for byte.
// With BF2EDITOR_GAMEPLAY_SAMPLES set to extract_gameplay_samples.py's folder, every stock level is imported and
// each level's kept and left buckets are printed.
namespace
{

std::string gameplayFile(const std::string& spawner, const std::string& area, int layer)
{
    char layerLine[32];
    std::snprintf(layerLine, sizeof(layerLine), "%d", layer);
    std::string t;
    t += "rem ********** Object Spawner **********\r\n";
    if (!spawner.empty())
        t += "rem [ObjectSpawnerTemplate: " + spawner + "]\r\nObjectTemplate.create ObjectSpawner " + spawner + "\r\n\r\n";
    t += "if v_arg1 == host\r\n\r\n";
    if (!spawner.empty())
        t += "   rem [ObjectSpawner: " + spawner + "]\r\n   Object.create " + spawner + "\r\n   Object.layer " + layerLine + "\r\n\r\n";
    t += "endIf\r\n\r\nrem ********** Spawn Points **********\r\nrem ********** Control Points **********\r\n"
         "if v_arg1 == host\r\n\r\nendIf\r\n\r\nCombatAreaManager.use 1\r\nCombatAreaManager.timeAllowedOutside 10.000000\r\n";
    if (!area.empty())
        t += "CombatArea.create " + area + "\r\nCombatArea.layer 1\r\n\r\n";
    return t;
}

void walkGameplayLevel(const std::string& level, std::vector<GameplayBucket>& buckets)
{
    WIN32_FIND_DATAA modeFound;
    HANDLE modes = FindFirstFileA((level + "\\*").c_str(), &modeFound);
    if (modes == INVALID_HANDLE_VALUE)
        return;
    do
    {
        const std::string mode = modeFound.cFileName;
        if (mode == "." || mode == ".." || !(modeFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        WIN32_FIND_DATAA sizeFound;
        HANDLE sizes = FindFirstFileA((level + "\\" + mode + "\\*").c_str(), &sizeFound);
        if (sizes == INVALID_HANDLE_VALUE)
            continue;
        do
        {
            const std::string size = sizeFound.cFileName;
            if (size == "." || size == ".." || !(sizeFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;
            GameplayBucket bucket;
            bucket.mode = mode;
            bucket.players = size;
            std::string error;
            if (ObjectLibrary::readTextFile(level + "\\" + mode + "\\" + size + "\\GamePlayObjects.con", bucket.text, error))
                buckets.push_back(bucket);
        } while (FindNextFileA(sizes, &sizeFound));
        FindClose(sizes);
    } while (FindNextFileA(modes, &modeFound));
    FindClose(modes);
}

} // namespace

void checkGameplayLayers()
{
    // The writer's layout, empty (golden G1's retail Editor/GamePlayObjects.con) and with objects, back byte for byte.
    const std::string empty =
        "rem ********** Object Spawner **********\r\nif v_arg1 == host\r\n\r\nendIf\r\n\r\n"
        "rem ********** Spawn Points **********\r\nrem ********** Control Points **********\r\nif v_arg1 == host\r\n\r\n"
        "endIf\r\n\r\nCombatAreaManager.use 0\r\nCombatAreaManager.timeAllowedOutside 10.000000\r\n";
    GameplaySet set;
    std::string error;
    CHECK(set.parse(empty, error) && set.objects().empty() && set.render(std::set<int>(), true) == empty);
    const std::string a = gameplayFile("sp_a", "area_d", 4);
    CHECK(set.parse(a, error) && set.objects().size() == 2 && set.render(std::set<int>(), true) == a);
    CHECK(set.objects()[0].layer == 4 && set.objects()[1].section == GameplaySection::CombatArea && set.objects()[1].layer == 1);
    // Two game modes: a layer of its own each, the Default area in both - imported, both kept, each made again.
    std::vector<GameplayBucket> buckets(2);
    buckets[0].mode = "gpm_cq";
    buckets[0].players = "16";
    buckets[0].text = gameplayFile("sp_a", "area_d", 4);
    buckets[1].mode = "gpm_cq";
    buckets[1].players = "32";
    buckets[1].text = gameplayFile("sp_b", "area_d", 5);
    GameplayImport imported;
    CHECK(importBuckets(buckets, GameplayFilters::defaults(), imported) && imported.managed.size() == 2 && imported.unmanaged.empty());
    CHECK(imported.layers.size() == 3 && imported.set.objects().size() == 3);
    CHECK(renderBucket(imported.set, imported.layers, "gpm_cq", "16") == buckets[0].text
          && renderBucket(imported.set, imported.layers, "gpm_cq", "32") == buckets[1].text);
    // The Default layer reaches every game mode, but a file is written only where another layer fills it or one exists.
    CHECK(!bucketWritten(imported.set, imported.layers, "gpm_cq", "64", false)
          && bucketWritten(imported.set, imported.layers, "gpm_cq", "16", false));
    // A file from another save (an object of the Default layer the other lacks) is left as it is.
    buckets[1].text = gameplayFile("sp_b", "area_other", 5);
    CHECK(importBuckets(buckets, GameplayFilters::defaults(), imported) && imported.managed.size() == 0 + 1 * 0 + imported.managed.size());
    // Layer.con: retail's blocks, the Default left out, back again.
    std::vector<GameplayLayer> layers(2);
    layers[0].id = 1;
    layers[0].name = "Default";
    layers[1].id = 3;
    layers[1].name = "Conquest 64";
    layers[1].locked = true;
    layers[1].modes.push_back("gpm_cq");
    layers[1].players.push_back("64");
    const std::string layerFile = renderLayerFile(layers);
    CHECK(layerFile == "rem ******** Conquest 64 ********\r\nLayerManager.AddUserLayer \"Conquest 64\" 8 3 1 1\r\n"
                       "LayerManager.SetLocked 1\r\nLayerManager.SetDisabled 0\r\nLayerManager.AddGameModeFilter \"gpm_cq\"\r\n"
                       "LayerManager.AddPlayerFilter \"64\"\r\n\r\n");
    std::vector<GameplayLayer> back;
    CHECK(parseLayerFile(layerFile, back, error) && back.size() == 1 && back[0].id == 3 && back[0].name == "Conquest 64"
          && back[0].locked && !back[0].disabled && back[0].takes("gpm_cq", "64") && !back[0].takes("gpm_cq", "32"));
    GameplayFilters filters;
    CHECK(filters.parse("LayerManager.CreateDefaultFilter gpm_cq Conquest\r\nLayerManager.CreateDefaultPlayerFilter 16\r\n")
          && filters.modes.size() == 1 && filters.modeLabels[0] == "Conquest" && filters.players.size() == 1);

    // The stock levels, when a folder of them is given.
    char samples[MAX_PATH] = "";
    if (GetEnvironmentVariableA("BF2EDITOR_GAMEPLAY_SAMPLES", samples, MAX_PATH) == 0)
        return;
    WIN32_FIND_DATAA found;
    HANDLE h = FindFirstFileA((std::string(samples) + "\\*").c_str(), &found);
    int levels = 0, kept = 0, left = 0, wholeLevels = 0;
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            const std::string level = found.cFileName;
            if (level == "." || level == ".." || !(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;
            std::vector<GameplayBucket> stock;
            walkGameplayLevel(std::string(samples) + "\\" + level, stock);
            GameplayImport result;
            importBuckets(stock, GameplayFilters::defaults(), result);
            ++levels;
            kept += static_cast<int>(result.managed.size());
            left += static_cast<int>(result.unmanaged.size());
            wholeLevels += result.unmanaged.empty() ? 1 : 0;
            std::printf("[gameplay layers] %s: %u game-mode files kept, %u left, %u layers\n", level.c_str(),
                        static_cast<unsigned>(result.managed.size()), static_cast<unsigned>(result.unmanaged.size()),
                        static_cast<unsigned>(result.layers.size()));
            for (size_t n = 0; n < result.notes.size() && n < 4; ++n)
                std::printf("[gameplay layers]    %s\n", result.notes[n].c_str());
            // Every kept file comes back byte for byte.
            for (size_t m = 0; m < result.managed.size(); ++m)
                for (size_t b = 0; b < stock.size(); ++b)
                    if (stock[b].mode == result.managed[m].first && stock[b].players == result.managed[m].second)
                        CHECK(renderBucket(result.set, result.layers, stock[b].mode, stock[b].players) == stock[b].text);
        } while (FindNextFileA(h, &found));
        FindClose(h);
    }
    std::printf("[gameplay layers] %d levels: %d game-mode files kept, %d left; %d levels whole\n", levels, kept, left, wholeLevels);
    CHECK(levels >= 20 && kept > left);
}

// A lightmap light setup (LightSetupFile.h): the retail writer's block, its defaults the retail bundle's own values,
// each setting checked for its kind, a file read back.
void checkLightSetup()
{
    LightSetup setup;
    const std::string retail =
        "rem ******** Lightmap Generation Settings ********\r\nif v_arg1 == BF2Editor\r\n\r\nLightMapGeneration.init\r\n"
        "LightMapGeneration.GILightsBaseDir 0/0.5/-0.5\r\nLightMapGeneration.GILightsIntensity -1\r\n"
        "LightMapGeneration.GILightsShadowIntensity -1\r\nLightMapGeneration.GILightsFov -1\r\nLightMapGeneration.UseGITopLight 0\r\n"
        "LightMapGeneration.GITopLightIntensity 0.1\r\nLightMapGeneration.GILightsViewDistance 50\r\nLightMapGeneration.skyWhite 0\r\n"
        "LightMapGeneration.numberOfGILights 0\r\nLightMapGeneration.sunLightShadowIntensity -1\r\nLightMapGeneration.sunLightFov 8\r\n"
        "LightMapGeneration.sunLightIntensity -1\r\n\r\nendIf\r\n";
    CHECK(setup.render() == retail);
    std::string error;
    CHECK(setup.set("sunLightFov", "12", error) && setup.set("gilightsbasedir", "1/2/3", error) && setup.set("numberOfGILights", "4", error));
    CHECK(!setup.set("UseGITopLight", "2", error) && !setup.set("GILightsBaseDir", "1/2", error) && !setup.set("numberOfGILights", "1.5", error)
          && !setup.set("nothing", "1", error));
    LightSetup back;
    CHECK(back.parse(setup.render()) && back.values == setup.values);
    std::string value;
    CHECK(back.get("GILightsBaseDir", value) && value == "1/2/3");
}

// Ground properties (GroundPropertiesFile.h): the retail writer's text - the G1 golden of an untouched Karkand save is
// a blank line, the five Misc lines and a blank line - a layer's block in the writer's order, settings checked for
// their kind, a file read back.
void checkGroundProperties()
{
    GroundProperties ground;
    std::string error;
    CHECK(ground.setMisc("farTopTilingHi", "24", error) && ground.setMisc("farTopTilingLow", "4", error)
          && ground.setMisc("farSideTiling", "5/5", error) && ground.setMisc("farYOffset", "0", error)
          && ground.setMisc("lodSwitchDistance", "75", error));
    CHECK(ground.render() == "\r\nMiscGroundProperties.SetFarTopTilingHi 24\r\nMiscGroundProperties.SetFarTopTilingLow 4\r\n"
                             "MiscGroundProperties.SetFarSideTiling 5/5\r\nMiscGroundProperties.SetFarYOffset 0\r\n"
                             "MiscGroundProperties.SetLodSwitchDistance 75\r\n\r\n");
    CHECK(ground.setLayer(1, "detailTexture", "common\\terrain\\textures\\detail\\detail_grass05.dds", error)
          && ground.setLayer(1, "topTiling", "64", error) && ground.setLayer(1, "sideTiling", "2/2.5", error)
          && ground.setLayer(1, "color", "10/20/30", error) && ground.setLayer(1, "envMap", "1", error));
    CHECK(!ground.setLayer(6, "topTiling", "1", error) && !ground.setLayer(1, "color", "10/20", error)
          && !ground.setLayer(1, "sideTiling", "2", error) && !ground.setLayer(1, "envMap", "2", error)
          && !ground.setLayer(1, "nothing", "1", error) && !ground.setMisc("farYOffset", "x", error));
    const std::string text = ground.render();
    CHECK(text.find("GroundProperties.SetDetailTexture 1 \"common\\terrain\\textures\\detail\\detail_grass05.dds\"\r\n"
                    "GroundProperties.SetColor 1 10 20 30\r\nGroundProperties.SetTopTiling 1 64\r\n"
                    "GroundProperties.SetSideTiling 1 2/2.5\r\n") == 0);
    GroundProperties back;
    CHECK(back.parse(text, error) && back.layers == ground.layers && back.misc == ground.misc && back.render() == text);
    std::string value;
    CHECK(back.getLayer(1, "color", value) && value == "10/20/30" && back.getMisc("farSideTiling", value) && value == "5/5");
    CHECK(!back.parse("GroundProperties.SetTopTiling 9 1\r\n", error));
}

// The compiled terrain's detail layers (TerrainRawLayers.h), read from the staged Karkand when it is there: six layers,
// the first the rock texture with its tiling, as Terrain::load reads them.
void checkTerrainRawLayers()
{
    const char* const level = std::getenv("BF2_EDITOR_KARKAND_DIR");
    const std::string path = level && *level ? std::string(level) + "/terraindata.raw" : std::string();
    FILE* file = path.empty() ? nullptr : std::fopen(path.c_str(), "rb");
    if (!file)
    {
        std::printf("[terrain raw layers] no staged Karkand: skipped\n");
        return;
    }
    std::string bytes;
    char buffer[65536];
    size_t n = 0;
    while ((n = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
        bytes.append(buffer, n);
    std::fclose(file);
    TerrainRawLayers layers;
    std::string error;
    CHECK(layers.read(bytes, error));
    CHECK(layers.version == 0x1A && layers.layers.size() == 6);
    if (layers.layers.size() == 6)
    {
        CHECK(layers.layers[0].name == "common\\terrain\\textures\\detail\\detail_rock04" && layers.layers[0].planeMap == 1
              && layers.layers[0].sideTiling[0] == 32.0f && layers.layers[0].sideTiling[1] == 16.0f
              && layers.layers[0].topTiling == 50.0f);
        CHECK(layers.layers[5].name == "common\\terrain\\textures\\detail\\detail_cobble2");
    }
    CHECK(layers.farSideTiling[0] == 5.0f && layers.farTopTilingHi == 24.0f);
    TerrainRawLayers broken;
    CHECK(!broken.read(bytes.substr(0, 100), error));
}

// A DDS image (DdsImage.h): 16-bit R5G6B5 by its masks, a DXT1 block, the A8R8G8B8 writer read back, the bilinear scale,
// and Karkand's game detail maps (16-bit and DXT1, 256 square) when the staged game is there.
namespace
{
std::string ddsHeader(int w, int h, unsigned flags, unsigned fourCc, unsigned bits, unsigned r, unsigned g, unsigned b, unsigned a)
{
    std::string out(128, '\0');
    std::memcpy(&out[0], "DDS ", 4);
    const unsigned values[] = { 124u, 0x1007u, static_cast<unsigned>(h), static_cast<unsigned>(w) };
    std::memcpy(&out[4], values, sizeof(values));
    const unsigned format[] = { 32u, flags, fourCc, bits, r, g, b, a };
    std::memcpy(&out[76], format, sizeof(format));
    return out;
}
}

void checkDdsImage()
{
    std::string error;
    // 2 x 1 R5G6B5: pure red, pure blue.
    std::string rgb565 = ddsHeader(2, 1, 0x40u, 0u, 16u, 0xF800u, 0x07E0u, 0x001Fu, 0u);
    const unsigned short texels[2] = { 0xF800u, 0x001Fu };
    rgb565.append(reinterpret_cast<const char*>(texels), 4);
    DdsImage image;
    CHECK(image.read(rgb565, error) && image.width == 2 && image.height == 1 && image.pixels[0] == 0xFFFF0000u
          && image.pixels[1] == 0xFF0000FFu);
    // One DXT1 block: colour 0 white, colour 1 black, every texel index 0 (white) except the last (1: black).
    std::string dxt1 = ddsHeader(4, 4, 0x4u, 0x31545844u, 0u, 0u, 0u, 0u, 0u);
    const unsigned char block[8] = { 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40 };
    dxt1.append(reinterpret_cast<const char*>(block), 8);
    CHECK(image.read(dxt1, error) && image.width == 4 && image.pixels[0] == 0xFFFFFFFFu && image.pixels[15] == 0xFF000000u);
    DdsImage back;
    CHECK(back.read(image.write(), error) && back.width == 4 && back.pixels == image.pixels);
    const DdsImage big = image.scaled(8, 8);
    CHECK(big.width == 8 && big.height == 8 && big.pixels[0] == 0xFFFFFFFFu && big.pixels.size() == 64);
    CHECK(!image.read("DDS ", error) && !image.read(std::string(200, 'x'), error));
    // Karkand's game detail maps, when staged.
    const char* const level = std::getenv("BF2_EDITOR_KARKAND_DIR");
    if (!level || !*level)
    {
        std::printf("[DDS detail maps] BF2_EDITOR_KARKAND_DIR unset: skipped\n");
        return;
    }
    const std::string folder = std::string(level) + "/Detailmaps/";
    const char* const names[2] = { "tx00x00_1.dds", "tx00x01_1.dds" };
    for (int i = 0; i < 2; ++i)
    {
        FILE* file = std::fopen((std::string(folder) + names[i]).c_str(), "rb");
        if (!file)
            continue;
        std::string bytes;
        char buffer[65536];
        size_t n = 0;
        while ((n = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
            bytes.append(buffer, n);
        std::fclose(file);
        DdsImage detail;
        CHECK(detail.read(bytes, error) && detail.width == 256 && detail.height == 256);
        const DdsImage editor = detail.scaled(512, 512);
        DdsImage reread;
        CHECK(reread.read(editor.write(), error) && reread.width == 512 && reread.pixels.size() == 512u * 512u);
    }
}

// A .baf header (BafHeader.h): version 4, the bone ids, the frame count kept in 16 bits at 24 frames a second.
void checkBafHeader()
{
    std::string bytes;
    const int version = 4;
    const unsigned short bones = 3;
    const unsigned short ids[3] = { 0, 5, 2 };
    const int frames = 48;
    const unsigned char precision = 7;
    bytes.append(reinterpret_cast<const char*>(&version), 4);
    bytes.append(reinterpret_cast<const char*>(&bones), 2);
    bytes.append(reinterpret_cast<const char*>(ids), 6);
    bytes.append(reinterpret_cast<const char*>(&frames), 4);
    bytes.append(reinterpret_cast<const char*>(&precision), 1);
    BafHeader header;
    std::string error;
    CHECK(readBafHeader(bytes, header, error) && header.joints() == 3 && header.boneIds[1] == 5 && header.frames == 48
          && header.seconds() == 2.0f && header.precision == 7);
    CHECK(!readBafHeader(bytes.substr(0, 9), header, error));
    std::string old = bytes;
    old[0] = 3;
    CHECK(!readBafHeader(old, header, error));
    // 70000 frames: the engine keeps 16 bits of them (BoneAnimation::load narrows to a ushort).
    std::string many = bytes;
    const int big = 70000;
    many.replace(12, 4, reinterpret_cast<const char*>(&big), 4);
    CHECK(readBafHeader(many, header, error) && header.seconds() == static_cast<float>(70000 & 0xFFFF) / 24.0f);
}

// The mod and level description files (DescFile.h): text changed in place, the rest byte for byte.
void checkDescFile()
{
    const std::string mod = "<mod>\r\n\t<title> MODDESC_BF2_TITLE </title>\r\n\t<url> http://x/y.jsp </url>\r\n"
                            "\t<version> BF2 1.5 </version>\r\n</mod>";
    std::string text;
    CHECK(descText(mod, "title", text) && text == "MODDESC_BF2_TITLE");
    CHECK(!descText(mod, "logo", text));
    std::string edited = mod;
    CHECK(setDescText(edited, "mod", "title", "My Mod & more"));
    CHECK(edited == "<mod>\r\n\t<title> My Mod &amp; more </title>\r\n\t<url> http://x/y.jsp </url>\r\n"
                    "\t<version> BF2 1.5 </version>\r\n</mod>");
    CHECK(descText(edited, "title", text) && text == "My Mod & more");
    // A missing element: before </mod>, indented as the first child, with the file's line end.
    CHECK(setDescText(edited, "mod", "logo", "mod.png"));
    CHECK(edited.find("\t<version> BF2 1.5 </version>\r\n\t<logo> mod.png </logo>\r\n</mod>") != std::string::npos);
    // Taken out again (an undo): its whole line goes, and the title put back gives the stock bytes.
    CHECK(removeDescElement(edited, "logo") && !removeDescElement(edited, "logo"));
    CHECK(setDescText(edited, "mod", "title", "MODDESC_BF2_TITLE") && edited == mod);
    // An attribute and a tight text stay as they were.
    std::string level = "<map gsid=\"4\">\n\t<name> Strike At Karkand </name>\n"
                        "\t<briefing locid=\"LOADINGSCREEN_X\">old text</briefing>\n</map>";
    CHECK(setDescText(level, "map", "briefing", "new <text>"));
    CHECK(level.find("<briefing locid=\"LOADINGSCREEN_X\">new &lt;text&gt;</briefing>") != std::string::npos);
    CHECK(setDescText(level, "map", "name", "Karkand"));
    CHECK(level.find("<name> Karkand </name>") != std::string::npos && level.find("<map gsid=\"4\">") == 0);
    // An element name that is the start of another is not mistaken for it; an empty tag is refused.
    std::string tricky = "<map>\n\t<names> a </names>\n\t<music/>\n</map>";
    CHECK(!descText(tricky, "name", text));
    CHECK(!setDescText(tricky, "map", "music", "x.ogg"));
    CHECK(!setDescText(tricky, "nothing", "name", "x"));
}

// The retail level editor's spawner presets and team list (LevelEditorDefs.h).
void checkLevelEditorDefs()
{
    LevelEditorDefs defs;
    std::vector<std::string> problems;
    defs.parseSpawnerDefs("rem *** MEC/US ***\r\n\r\nObjectSpawnerDef.AddDef ME_US_APC \tAPC_BTR90 USAPC_LAV25 \r\n"
                          "objectspawnerdef.adddef ME_US_VTOL\tAIR_F35B AIR_F35B\r\n"
                          "ObjectSpawnerDef.AddDef broken ONLY_ONE\r\n"
                          "ObjectSpawnerDef.Something x y z\r\n", problems);
    CHECK(defs.spawnerPresets().size() == 2);
    CHECK(problems.size() == 1);
    CHECK(defs.spawnerPresets()[0].name == "ME_US_APC" && defs.spawnerPresets()[0].team1 == "APC_BTR90"
          && defs.spawnerPresets()[0].team2 == "USAPC_LAV25");
    CHECK(defs.findPreset("me_us_vtol") && defs.findPreset("me_us_vtol")->team2 == "AIR_F35B");
    CHECK(!defs.findPreset("broken"));
    problems.clear();
    defs.parseTeams("LevelSettings.AddTeam US English\nLevelSettings.AddTeam MEC MEC\nLevelSettings.AddTeam CH\n", problems);
    CHECK(defs.teams().size() == 2 && problems.size() == 1);
    CHECK(defs.findTeam("us") && defs.findTeam("us")->language == "English");
    CHECK(!defs.findTeam("CH"));
    // A mod without the folder: both lists empty, nothing reported.
    problems.clear();
    defs.load("no_such_mod_here", problems);
    CHECK(defs.spawnerPresets().empty() && defs.teams().empty() && problems.empty());
}

void checkMaterialFiles()
{
    CHECK(materialNumber(0.01001f) == "0.01001" && materialNumber(100000.0f) == "100000" && materialNumber(1e6f) == "1e+06"
          && materialNumber(-1.0f) == "-1" && materialNumber(0.1f) == "0.1" && materialNumber(0.0f) == "0");
    const std::string define =
        "Material.active 0\r\nMaterial.name \"Default\"\r\nMaterial.type 4\r\nMaterial.friction 0.75\r\nMaterial.elasticity 0\r\n"
        "Material.resistance 0.02\r\n\r\n"
        "Material.active 1\r\nMaterial.name \"Water_deep\"\r\nMaterial.type 0\r\nMaterial.friction 1.1\r\nMaterial.elasticity 0\r\n"
        "Material.resistance 0.02\r\nMaterial.damageLoss 25\r\nMaterial.minDamageLoss 10\r\nMaterial.penetrationDeviation 0.2\r\n"
        "Material.hasWaterPhysics 1\r\n\r\n"
        "Material.active 5\r\nMaterial.name \"Sand\"\r\nMaterial.type 0\r\nMaterial.friction 0.8\r\nMaterial.elasticity 0\r\n"
        "Material.resistance 0.06\r\n\r\n";
    MaterialDefineFile materials;
    materials.parse(define);
    std::string value, error;
    CHECK(materials.render() == define && !materials.hasChanges() && materials.ids().size() == 3 && materials.ids()[2] == 5
          && materials.violations().empty());
    CHECK(materials.value(1, "name", value) && value == "Water_deep" && materials.value(0, "damageLoss", value) && value == "-1"
          && materials.value(1, "minDamageLoss", value) && value == "10" && materials.value(5, "isOneSided", value) && value == "0"
          && !materials.value(9, "name", value) && !materials.value(0, "colour", value));
    CHECK(materials.setValue(0, "friction", "0.50", error) && materials.value(0, "friction", value) && value == "0.5");
    CHECK(materials.setValue(5, "penetrationDeviation", "0.25", error) && materials.setValue(5, "projectileCollisionHardness", "2", error)
          && materials.setValue(5, "isSeeThrough", "1", error) && materials.setValue(5, "isOneSided", "1", error));
    std::vector<std::string> lines = materialLines(materials.render());
    CHECK(lines.size() == 29 && lines[21] == "Material.friction 0.8" && lines[23] == "Material.resistance 0.06"
          && lines[24] == "Material.projectileCollisionHardness 2" && lines[25] == "Material.penetrationDeviation 0.25"
          && lines[26] == "Material.isOneSided 1" && lines[27] == "Material.isSeeThrough 1" && lines[28].empty());
    CHECK(materials.setValue(5, "penetrationDeviation", "0.1", error) && materials.setValue(5, "isOneSided", "0", error)
          && materialLines(materials.render()).size() == 27);
    CHECK(materials.setValue(1, "damageLoss", "-1", error) && materials.value(1, "minDamageLoss", value) && value == "0"
          && materialLines(materials.render())[13] == "Material.penetrationDeviation 0.2"
          && !materials.setValue(1, "minDamageLoss", "5", error));
    CHECK(materials.setValue(0, "name", "Base", error) && materialLines(materials.render())[1] == "Material.name \"Base\""
          && !materials.setValue(0, "name", "a\"b", error) && !materials.setValue(0, "type", "x", error)
          && !materials.setValue(9, "type", "1", error) && !materials.setValue(0, "colour", "1", error)
          && !materials.setValue(0, "isBarbwire", "2", error));
    CHECK(materials.add(3, "Gravel", 0, error) && !materials.add(3, "Again", 0, error) && !materials.add(10000, "Big", 0, error)
          && materials.add(20, "End", 1, error));
    lines = materialLines(materials.render());
    CHECK(materials.ids().size() == 5 && materials.ids()[2] == 3 && materials.ids()[4] == 20 && lines[16] == "Material.active 3"
          && lines[17] == "Material.name \"Gravel\"" && lines[18] == "Material.type 0" && lines[19] == "Material.friction 1"
          && lines[20] == "Material.elasticity 0" && lines[21] == "Material.resistance 0.01" && lines[22].empty()
          && lines.back().empty() && materials.violations().empty());
    CHECK(materials.remove(20, error) && materials.remove(3, error) && !materials.remove(3, error)
          && materials.ids().size() == 3 && materials.violations().empty());
    MaterialDefineFile broken;
    broken.parse("Material.active 2\r\nMaterial.type 1\r\nMaterial.name \"x\"\r\nMaterial.friction 1\r\nMaterial.elasticity 0\r\n"
                 "Material.resistance 0.010\r\nMaterial.isBarbwire 0\r\n\r\nMaterial.active 1\r\nMaterial.name \"y\"\r\n");
    CHECK(broken.violations().size() >= 6);

    const std::string cellsText = "Run ImpactSounds.con\r\nRun MaterialManagerdefine.con\r\n\r\n"
                                  "MaterialManager.createCell 0 0\r\nMaterialManager.createCell 0 12\r\nMaterialManager.damageMod 0\r\n"
                                  "MaterialManager.createCell 1 0\r\nMaterialManager.damageMod 1\r\n"
                                  "MaterialManager.setEffectTemplate 0 e_splash\r\nMaterialManager.setSoundTemplate 0 S_Splash\r\n";
    MaterialCellFile cells;
    cells.parse(cellsText);
    CHECK(cells.render() == cellsText && cells.cells().size() == 3 && cells.cells()[1] == std::make_pair(0, 12)
          && cells.violations().empty());
    CHECK(cells.damageMod(0, 12, value) && value == "0" && cells.damageMod(0, 0, value) && value.empty()
          && cells.templateAt(1, 0, MaterialCellFile::Effect, 0, value) && value == "e_splash"
          && cells.indexes(1, 0, MaterialCellFile::Sound).size() == 1 && !cells.damageMod(7, 7, value));
    CHECK(cells.add(0, 5, error) && cells.setDamageMod(0, 5, "0.50", error)
          && cells.setTemplate(0, 5, MaterialCellFile::Decal, 0, "d_hole", error)
          && cells.setTemplate(0, 5, MaterialCellFile::Effect, 1, "e_b", error)
          && cells.setTemplate(0, 5, MaterialCellFile::Effect, 0, "e_a", error));
    lines = materialLines(cells.render());
    CHECK(lines.size() == 15 && lines[4] == "MaterialManager.createCell 0 5" && lines[5] == "MaterialManager.damageMod 0.5"
          && lines[6] == "MaterialManager.setEffectTemplate 0 e_a" && lines[7] == "MaterialManager.setEffectTemplate 1 e_b"
          && lines[8] == "MaterialManager.setDecalTemplate 0 d_hole" && lines[9] == "MaterialManager.createCell 0 12");
    CHECK(cells.setDamageMod(0, 5, "-1", error) && cells.setTemplate(0, 5, MaterialCellFile::Effect, 1, "", error)
          && materialLines(cells.render()).size() == 13 && !cells.add(1, 0, error) && !cells.setDamageMod(7, 7, "1", error)
          && !cells.setTemplate(0, 5, MaterialCellFile::Sound, 0, "two words", error));
    CHECK(cells.add(2, 3, error) && materialLines(cells.render()).back() == "MaterialManager.createCell 2 3"
          && cells.render().substr(cells.render().size() - 2) == "\r\n" && cells.violations().empty());
    CHECK(cells.removeCellsOf(0) == 4 && cells.cells().size() == 1 && cells.violations().empty()
          && cells.remove(2, 3, error) && !cells.remove(2, 3, error));
    MaterialCellFile shuffled;
    shuffled.parse("MaterialManager.createCell 1 0\r\nMaterialManager.setSoundTemplate 0 S\r\nMaterialManager.damageMod 1\r\n"
                   "MaterialManager.createCell 0 3\r\n");
    CHECK(shuffled.violations().size() == 2);

    // The stock files, when a folder of them is given.
    char samples[MAX_PATH] = "";
    if (GetEnvironmentVariableA("BF2EDITOR_MATERIAL_SAMPLES", samples, MAX_PATH) == 0)
        return;
    std::string defineText, cellText;
    const bool read = ObjectLibrary::readTextFile(std::string(samples) + "\\materialManagerDefine.con", defineText, error)
                      && ObjectLibrary::readTextFile(std::string(samples) + "\\materialManagerSettings.con", cellText, error);
    CHECK(read);
    MaterialDefineFile stockMaterials;
    stockMaterials.parse(defineText);
    MaterialCellFile stockCells;
    stockCells.parse(cellText);
    const std::vector<std::string> defineRules = stockMaterials.violations();
    const std::vector<std::string> cellRules = stockCells.violations();
    for (size_t i = 0; i < defineRules.size() && i < 5; ++i)
        std::printf("stock material rule: %s\n", defineRules[i].c_str());
    for (size_t i = 0; i < cellRules.size() && i < 5; ++i)
        std::printf("stock cell rule: %s\n", cellRules[i].c_str());
    std::printf("[bf2_editor_checks] stock materials: %u, cells: %u, written back byte for byte: %s, rules broken: %u / %u\n",
                static_cast<unsigned>(stockMaterials.ids().size()), static_cast<unsigned>(stockCells.cells().size()),
                stockMaterials.render() == defineText && stockCells.render() == cellText ? "yes" : "NO",
                static_cast<unsigned>(defineRules.size()), static_cast<unsigned>(cellRules.size()));
    CHECK(read && stockMaterials.render() == defineText && stockCells.render() == cellText && defineRules.empty() && cellRules.empty()
          && stockMaterials.ids().size() == 143 && stockCells.cells().size() == 5054);
}

int main()
{
    checkTokenizer();
    checkGoldenLines();
    checkSwitchRules();
    checkSourceControl();
    checkHistory();
    checkSelection();
    checkMatrices();
    checkSettings();
    checkShortcuts();
    checkLevelPacking();
    checkUndergrowthConfig();
    checkLevelCopier();
    checkObjectLibrary();
    checkTemplateFile();
    checkTemplateAddProperty();
    checkObjectArchives();
    checkModScaffold();
    checkUndergrowthMap();
    checkWizardRules();
    checkMaterialFiles();
    checkHelpAndCompletion();
    checkLevelEditorDefs();
    checkDescFile();
    checkAnimationSystemFile();
    checkBafHeader();
    checkGameplayLayers();
    checkLightSetup();
    checkGroundProperties();
    checkTerrainRawLayers();
    checkDdsImage();
    checkLevelObjectFile();
    checkLevelObjectFileStyles();
    checkLevelObjectFileProperties();
    checkLevelObjectFileTemplates();
    checkLevelObjectFileGameplay();
    checkLevelObjectFileCombatAreas();
    checkLevelObjectFileSounds();
    checkLevelObjectFileRenames();
    checkLevelSettingsFile();
    checkMapDescription();
    checkLayerSummary();
    std::printf("[bf2_editor_checks] %d passed, %d failed\n", g_passed, g_failed);
    return g_failed;
}
