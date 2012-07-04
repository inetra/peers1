#include "stdafx.h"
#include "ToolbarImages.h"

void ToolbarImages::init() {
  const string images = SETTING(TOOLBARIMAGE);
  if (images.empty()) {
    largeImages.CreateFromImage(IDB_TOOLBAR20, 24, 24, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
  }
  else {
    largeImages.CreateFromImage(Text::toT(images).c_str(), SETTING(TB_IMAGE_SIZE), 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
  }
  const string hotImages = SETTING(TOOLBARHOTIMAGE);
  if (hotImages.empty()) {
    largeImagesHot.CreateFromImage(IDB_TOOLBAR20_HOT, 24, 24, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
  }
  else {
    largeImagesHot.CreateFromImage(Text::toT(hotImages).c_str(), SETTING(TB_IMAGE_SIZE_HOT), 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
  }
}

void ToolbarImages::destroy() {
  largeImages.Destroy();
  largeImagesHot.Destroy();
}
