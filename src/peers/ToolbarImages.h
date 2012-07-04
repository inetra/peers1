class ToolbarImages {
private:
  CImageList largeImages, largeImagesHot;
  void destroy();
public:
  ~ToolbarImages() {
    destroy();
  }
  void init();
  HIMAGELIST getImages() const { return largeImages; }
  HIMAGELIST getHotImages() const { return largeImagesHot; }
};
