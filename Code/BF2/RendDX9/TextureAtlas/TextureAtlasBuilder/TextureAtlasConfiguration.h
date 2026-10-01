// Path:     [INFERRED] TextureAtlasBuilder/TextureAtlasConfiguration.h - no __FILE__ survives in the DLL.
// Contents: from TextureAtlasBuilder.dll 1.50; layout read from the constructor
//           (0x10008030), the destructor (0x10008070) and every accessor.

#ifndef TEXTUREATLASBUILDER_TEXTUREATLASCONFIGURATION_H
#define TEXTUREATLASBUILDER_TEXTUREATLASCONFIGURATION_H

#include "ITextureAtlas.h"

#include <string>
#include <vector>

namespace textureatlas {


// Atlas source groups and input/output paths. addGroup sets the insertion group;
// selectGroup does not change where subsequent textures are added.

class TextureAtlasConfiguration : public ITextureAtlasConfiguration
{
public:
    TextureAtlasConfiguration();
    virtual ~TextureAtlasConfiguration();

    virtual bool load(const char* tacPath);                                 // +0x04
    virtual void save(const char* tacPath);                                 // +0x08
    virtual void setOutputPath(const char* directory);                      // +0x0C
    virtual void setInputPath(const char* directory);                       // +0x10
    virtual const char* getInputPath();                                     // +0x14
    virtual const char* getOutputPath();                                    // +0x18
    virtual Group* selectGroup(const char* name);                           // +0x1C
    virtual Group* addGroup(const char* name, int width, int height,
                            const char* format, int miplevels, int border); // +0x20
    virtual TextureInfo* addTexture(const char* path, bool border);         // +0x24
    virtual GroupIter findGroup(const char* name);                          // +0x28
    virtual GroupIter groupsBegin();                                        // +0x2C
    virtual GroupIter groupsEnd();                                          // +0x30
    virtual GroupIter removeGroup(GroupIter group);                         // +0x34
    virtual void setErrorCallback(MessageFn callback);                      // +0x38
    virtual void setWarningCallback(MessageFn callback);                    // +0x3C

    // +0x40: fill `info` for `inputDirectory + name` from the image file; false when it cannot be read.
    virtual bool getTextureInfo(const std::string& inputDirectory, const std::string& name,
                                TextureInfo& info) = 0;

protected:
    std::string        m_inputPath;
    std::string        m_outputPath;
    std::vector<Group> m_groups;
    Group*             m_current;
    MessageFn          m_error;
    MessageFn          m_warning;
};

} // namespace textureatlas

#endif // TEXTUREATLASBUILDER_TEXTUREATLASCONFIGURATION_H
