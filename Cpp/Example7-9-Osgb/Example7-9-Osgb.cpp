#include <iostream>
#include <osg/PagedLod>
#include <osgDB/ReadFile>

using namespace std;

void TraversePagedLod(osg::PagedLOD* pagedLod, int printSpace);

void TraverseGroupNode(const std::string& filePath, int printSpace) {
  osg::Group* node = (osg::Group*)osgDB::readNodeFile(filePath);

  for (unsigned int i = 0; i < node->getNumChildren(); i++) {
    osg::PagedLOD* pagedLod = dynamic_cast<osg::PagedLOD*>(node->getChild(i));
    if (pagedLod) {
      TraversePagedLod(pagedLod, printSpace);
    }
  }
}

void TraversePagedLod(osg::PagedLOD* pagedLod, int printSpace) {
  const auto& rangList = pagedLod->getRangeList();
  unsigned int n = pagedLod->getNumFileNames();

  for (unsigned int i = 1; i < n; i++) {
    for (int j = 0; j < printSpace; j++) {
      cout << "    ";
    }
    cout << pagedLod->getFileName(i) << "  ";
    cout << pagedLod->getMinRange(i) << "  " << pagedLod->getMaxRange(i)
         << endl;

    std::string fileFullPath =
        pagedLod->getDatabasePath() + "/" + pagedLod->getFileName(i);
    TraverseGroupNode(fileFullPath, printSpace + 1);
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("请设置osgb文件路径！");
  }

  string osgPath = argv[1];
  if (osgPath.empty()) {
    printf("osgb文件路径为空！");
  }

  int printSpace = 0;
  TraverseGroupNode(osgPath, printSpace);

  return 0;
}
