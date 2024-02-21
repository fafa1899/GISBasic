#include <filesystem>
#include <iostream>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>

using namespace std;

void FindFolder(std::string strPath, std::vector<std::string>& vecFiles) {
  std::error_code ec;
  std::filesystem::path fsPath(strPath);
  if (!std::filesystem::exists(strPath, ec)) {
    return;
  }

  for (auto& itr : std::filesystem::directory_iterator(fsPath)) {
    if (std::filesystem::is_directory(itr.status())) {
      auto path = itr.path().string();
      // 将路径中的反斜杠替换为正斜杠
      std::replace(path.begin(), path.end(), '\\', '/');
      vecFiles.push_back(path);
    }
  }
}

//得到文件路径的文件名   C:\\b\\a(.txt) -> a
std::string DirOrPathGetName(std::string filePath) {
  size_t m = filePath.find_last_of('/');
  if (m == string::npos) {
    return filePath;
  }

  size_t p = filePath.find_last_of('.');
  if (p != string::npos && p > m)  //没有点号或者
  {
    filePath.erase(p);
  }

  std::string dirPath = filePath;
  dirPath.erase(0, m + 1);
  return dirPath;
}

void CreateObliqueIndexes(std::string fileDir) {
  osg::ref_ptr<osg::Group> group = new osg::Group();
  vector<string> subDirs;
  FindFolder(fileDir, subDirs);

  for (size_t i = 0; i < subDirs.size(); i++) {
    string path = subDirs[i] + "/Data/Model/Model.osgb";

    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(path);
    osg::ref_ptr<osg::PagedLOD> lod = new osg::PagedLOD();

    auto bs = node->getBound();
    auto c = bs.center();
    auto r = bs.radius();
    lod->setCenter(c);
    lod->setRadius(r);
    lod->setRangeMode(osg::LOD::RangeMode::PIXEL_SIZE_ON_SCREEN);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->getOrCreateStateSet();
    lod->addChild(geode.get());

    std::string relativeFilePath = "./" + DirOrPathGetName(subDirs[i]) +
                                   "/Data/Model/Model.osgb";  //相对路径

    lod->setFileName(0, "");
    lod->setFileName(1, relativeFilePath);

    lod->setRange(0, 0, 1.0);  //第一层不可见
    lod->setRange(1, 1.0, FLT_MAX);

    lod->setDatabasePath("");

    group->addChild(lod);
  }
  std::string outputLodFile = fileDir + "/Data.osgb";
  osgDB::writeNodeFile(*group, outputLodFile);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("请设置osgb文件路径！");
  }

  string fileDir = argv[1];
  if (fileDir.empty()) {
    printf("osgb文件路径为空！");
  }

  std::string outputLodFile = fileDir + "/Data.osgb";
  CreateObliqueIndexes(fileDir);

  osgViewer::Viewer viewer;
  osg::ref_ptr<osg::Node> node = osgDB::readRefNodeFile(outputLodFile);

  //去掉光照，便于显示
  node->getOrCreateStateSet()->setMode(
      GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

  viewer.setSceneData(node);

  return viewer.run();
}