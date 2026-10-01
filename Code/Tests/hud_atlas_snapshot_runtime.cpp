// [TEMP] [CHOSEN, not recovered] Actual atlas owning-copy helper regression.
// Including the production TU exposes its private helper without adding a test
// export. Strictly linked with the isolated native renderer objects except that
// TU's duplicate object. No factory, atlas parser, D3D device or cache release.
#include <cstdio>
#include <string>
#include <vector>
#include "../BF2/RendDX9/TextureManager.cpp"
int main()
{
    using namespace renddx9abi;
    unsigned checks=0,failures=0;
    auto check=[&](bool b,const char* label) {++checks;if(!b){++failures;std::printf("FAIL %s\n",label);}};
    HostStringList names;
    names.push_back("first.tai");
    names.push_back("a_long_atlas_name_beyond_small_string_storage.tai");
    names.push_back("first.tai");
    const auto initial=dice::hfe::dx9rend::copyAtlasNames(names);
    check(initial.size()==3,"snapshot preserves duplicates");
    std::vector<std::string> visited;
    for(const auto& name:initial) {
        visited.push_back(name);
        if(visited.size()==1) names.push_back("late.tai"); // reentrant loader-like mutation
    }
    check(visited==std::vector<std::string>({"first.tai","a_long_atlas_name_beyond_small_string_storage.tai","first.tai"}),
          "append during load does not extend current traversal");
    check(names.size()==4&&names.back()=="late.tai","live HUD append remains for next load");
    const auto next=dice::hfe::dx9rend::copyAtlasNames(names);
    check(next.size()==4&&next.back()=="late.tai","next load sees appended name");
    names.clear();
    names.push_back("replacement.tai");
    check(initial.front()=="first.tai"&&initial.size()==3,"clear of source does not invalidate snapshot");
    auto i=initial.begin();++i;
    check(*i=="a_long_atlas_name_beyond_small_string_storage.tai","long string buffer has owning lifetime");
    check(next.size()==4&&next.front()=="first.tai","independent snapshots survive source replacement");
    const auto replacement=dice::hfe::dx9rend::copyAtlasNames(names);
    check(replacement.size()==1&&replacement.front()=="replacement.tai","new copy reflects replacement source");
    names.clear();
    check(dice::hfe::dx9rend::copyAtlasNames(names).empty(),"empty snapshot remains empty");
    names.push_back(std::string("a\0b",3));
    const auto binary=dice::hfe::dx9rend::copyAtlasNames(names);
    check(binary.front().size()==3&&binary.front()[2]=='b',"string copy retains embedded NUL bytes");
    names.front()[2]='z';
    check(binary.front()[2]=='b',"source string mutation cannot modify copied payload");
    check(names.front()[2]=='z',"source remains independently writable");
    std::printf("Atlas snapshot: %u/%u checks passed; strict native x86 renderer link\n",checks-failures,checks);
    return failures?1:0;
}

