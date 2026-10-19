#ifdef _WIN32
#include <windows.h>
#endif
#include "catalog.hpp"
#include "workflow.hpp"
#include <cassert>
int main(){pv::Catalog c;c.add({"a.mp4","A",""});c.rename(0,"B");assert(c.items[0].name=="B");c.erase(0);assert(c.items.empty());assert(c.undo());assert(c.items[0].name=="B");assert(c.undo());assert(c.items[0].name=="A");assert(c.redo());bool bad=false;try{c.rename(0,"");}catch(...){bad=true;}assert(bad);auto p=std::filesystem::temp_directory_path()/"pv-gallery-test.json";c.save(p);pv::Catalog d;d.load(p);assert(d.items[0].name=="B");std::filesystem::remove(p);assert(pv::needsOptimization({3840,2160,30,40000000},8,8ull<<30));assert(pv::needsOptimization({1920,1080,60,10000000},2,4ull<<30));assert(!pv::needsOptimization({1280,720,30,4000000},8,8ull<<30));}
