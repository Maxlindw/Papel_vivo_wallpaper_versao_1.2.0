#pragma once
// Monochrome vector artwork, shipped as SVG and embedded PNG. No emoji/font fallback.
Gdiplus::Bitmap* loadArt(int);
bool galleryEditingForIcon();
Gdiplus::Bitmap* uiIcons[15]{};
Gdiplus::Bitmap* uiIcon(int id){if(id<201||id>215)return nullptr;auto&im=uiIcons[id-201];if(!im)im=loadArt(id);return im;}
int iconForButton(int id){switch(id){case ID_CHOOSE:return 201;case ID_APPLY:case 7010:return 202;case ID_PREVIEW:return 203;case ID_PAUSE:return 204;case ID_STOP:case 7014:case IDNO:case 603:return 205;case ID_GALLERY:return 206;case ID_REMOVE_BG:return 207;case ID_LOGS:return 208;case ID_LANGUAGE:return 209;case ID_ORIGINAL:return 211;case 7011:return galleryEditingForIcon()?215:212;case 7012:return 213;case 7013:return 214;case IDYES:return 215;default:return 210;}}
