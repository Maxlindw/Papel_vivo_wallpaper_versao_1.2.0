#pragma once
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
namespace pv {
inline std::string key(std::string v){while(!v.empty()&&isspace((unsigned char)v.back()))v.pop_back();while(!v.empty()&&isspace((unsigned char)v.front()))v.erase(v.begin());std::transform(v.begin(),v.end(),v.begin(),[](unsigned char c){return char(tolower(c));});return v;}
struct MediaItem{std::string path,name,thumbnail;};
inline void to_json(nlohmann::json&j,const MediaItem&m){j={{"path",m.path},{"name",m.name},{"thumbnail",m.thumbnail}};}
inline void from_json(const nlohmann::json&j,MediaItem&m){m.path=j.at("path");m.name=j.at("name");m.thumbnail=j.value("thumbnail","");}
struct Catalog{
 std::vector<MediaItem> items;std::vector<std::vector<MediaItem>> undoStack,redoStack;
 void checkpoint(){undoStack.push_back(items);if(undoStack.size()>100)undoStack.erase(undoStack.begin());redoStack.clear();}
 std::string unique(std::string n)const{auto orig=n;unsigned count=2;for(;;){bool exists=false;for(auto&m:items)if(key(m.name)==key(n))exists=true;if(!exists)return n;n=orig+" ("+std::to_string(count++)+")";}}
 void add(MediaItem m){for(auto&i:items)if(i.path==m.path)return;if(items.size()>=1000)throw std::runtime_error("Limite de 1000 mídias / 1000-media limit");checkpoint();m.name=unique(m.name);items.push_back(m);}
 void rename(size_t i,std::string name){if(i>=items.size()||key(name).empty()||name.size()>240)throw std::runtime_error("Nome inválido / Invalid name");for(size_t n=0;n<items.size();n++)if(n!=i&&key(items[n].name)==key(name))throw std::runtime_error("Nome duplicado / Duplicate name");checkpoint();items[i].name=name;}
 void erase(size_t i){if(i<items.size()){checkpoint();items.erase(items.begin()+i);}}
 bool undo(){if(undoStack.empty())return false;redoStack.push_back(items);items=undoStack.back();undoStack.pop_back();return true;}
 bool redo(){if(redoStack.empty())return false;undoStack.push_back(items);items=redoStack.back();redoStack.pop_back();return true;}
 void load(const std::filesystem::path&p){std::ifstream f(p);if(f){auto j=nlohmann::json::parse(f);auto next=j.get<std::vector<MediaItem>>();if(next.size()>1000)throw std::runtime_error("Catalog limit");items=std::move(next);}}
 void save(const std::filesystem::path&p){std::filesystem::create_directories(p.parent_path());auto tmp=p;tmp+=".tmp";{std::ofstream f(tmp,std::ios::binary);if(!f)throw std::runtime_error("Gallery write error");f<<nlohmann::json(items).dump(2);f.flush();if(!f)throw std::runtime_error("Gallery disk error");}
#ifdef _WIN32
 if(!MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Gallery atomic replace failed");
#else
 std::filesystem::rename(tmp,p);
#endif
 }
};
}
