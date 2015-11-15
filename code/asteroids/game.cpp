#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>

#include <CoreUtils/Graphics/Camera.h>

#include <asteroids/gameLoop.h>
#include <asteroids/level.h>

void mountAsteroidsDataDirs()
{
  auto appDir = ezOSFile::GetApplicationDirectory();

  // Shaders
  {
    ezStringBuilder shaderDir(appDir);
    shaderDir.PathParentDirectory();
    shaderDir.AppendPath("data", "shaders");

    // To be used as "<shader>sprite.vs"
    EZ_VERIFY(ezFileSystem::AddDataDirectory(shaderDir.GetData(), ezFileSystem::ReadOnly, "data", "shader").Succeeded(),
              "Failed to mount shaders directory.");
  }

  // Textures
  {
    ezStringBuilder texDir(appDir);
    texDir.PathParentDirectory();
    texDir.AppendPath("data", "textures");

    // To be used as "<texture>ship.dds"
    EZ_VERIFY(ezFileSystem::AddDataDirectory(texDir.GetData(), ezFileSystem::ReadOnly, "data", "texture").Succeeded(),
              "Failed to mount textures directory.");
  }
}

struct BasicLogging
{
  BasicLogging()
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  ~BasicLogging()
  {
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  }
};

struct FileLogging
{
  ezLogWriter::HTML htmlLog;

  FileLogging()
  {
    EZ_VERIFY(ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory(),
                                             ezFileSystem::AllowWrites, "log", "log").Succeeded(),
              "Failed to mount shaders directory.");

    htmlLog.BeginLog("<log>asteroidsLog.html", "Asteroids");
  }

  ~FileLogging()
  {
    htmlLog.EndLog();
  }
};

struct Cam
{
  Cam(kr::Borrowed<kr::Window> window)
  {
    auto size = window->getClientAreaSize();
    this->aspect = static_cast<float>(size.width) / size.height;

    this->cam.SetCameraMode(ezCamera::OrthoFixedWidth,       // Using an orthographic cam.
                            static_cast<float>(size.width),  // Width of the orthographic cam.
                            0.1f,                            // Near plane.
                            1.0f);                           // Far plane.
    this->cam.LookAt(ezVec3(0, 0, -0.5f), // Camera Position.
                     ezVec3(0, 0, 0),    // Target Position.
                     ezVec3(0, 1, 0));   // Up Vector.

    kr::Renderer::addExtractionListener({ &Cam::extract, this });
  }

  ~Cam()
  {
    kr::Renderer::removeExtractionListener({ &Cam::extract, this });
  }

  void extract(kr::Renderer::Extractor& e)
  {
    kr::extract(e, this->cam, this->aspect);
  }

  float aspect;
  ezCamera cam;
};

kr::Owned<kr::Window> createAsteroidsWindow()
{
  ezWindowCreationDesc desc;
  desc.m_ClientAreaSize.width = 512;
  desc.m_ClientAreaSize.height = 512;
  desc.m_Title = "Asteroids - Powered by Krepel";

  return kr::Window::createAndOpen(desc);
}

/*
int main(int argc, char* argv[])
{
  {
    BasicLogging _basicLogging;

    ezStartup::StartupCore();
    KR_ON_SCOPE_EXIT{ ezStartup::ShutdownCore(); };

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    FileLogging _fileLogging;

    mountAsteroidsDataDirs();

    {
      GameLoopData gameLoop;

      // Window
      // ======
      auto window = createAsteroidsWindow();
      window->getEvent().AddEventHandler([&gameLoop](const kr::WindowEventArgs& e)
      {
        if(e.type == kr::WindowEventArgs::ClickClose)
        {
          gameLoop.stop = true;
        }
      });

      // Camera
      // ======
      Cam cam(window);

      // Engine Startup
      // ==============
      ezStartup::StartupEngine();
      KR_ON_SCOPE_EXIT{ ezStartup::ShutdownEngine(); };

      // Level Initialization
      // ====================
      ezRectFloat levelBounds(0,
                              0,
                              static_cast<float>(window->getClientAreaSize().width),
                              static_cast<float>(window->getClientAreaSize().height)
                              );
      levelBounds.x = -0.5f * levelBounds.width;
      levelBounds.y = -0.5f * levelBounds.height;
      level::initialize(levelBounds);
      KR_ON_SCOPE_EXIT{ level::shutdown(); };

      // Game Loop
      // =========
      auto now = ezTime::Now();
      while(true)
      {
        gameLoop.dt = ezTime::Now() - now;

        level::update(gameLoop);
        if(gameLoop.stop) break;

        kr::processWindowMessages(window);
        if(gameLoop.stop) break;

        kr::Renderer::extract();
        kr::Renderer::update(gameLoop.dt, window);

        now += gameLoop.dt;
      }
    }
  }

  return 0;
}
*/

#include <krEngine/game.h>

class AsteroidsModule : public kr::DefaultMainModule
{
public:
  Cam* m_pCam;

  AsteroidsModule()
  {
    m_windowDesc.m_ClientAreaSize.width = 512;
    m_windowDesc.m_ClientAreaSize.height = 512;
    m_windowDesc.m_Title = "Asteroids - Powered by Krepel";

    EZ_VERIFY(ezFileSystem::AddDataDirectory(kr::makePath(kr::defaultRoot(), "data", "textures"), ezFileSystem::ReadOnly, "", "texture").Succeeded(),
              "Failed to mount textures directory.");

    EZ_VERIFY(ezFileSystem::AddDataDirectory(kr::makePath(kr::defaultRoot(), "data", "shaders"), ezFileSystem::ReadOnly, "", "shader").Succeeded(),
              "Failed to mount shaders directory.");
  }

  virtual void OnEngineStartup() override
  {
    kr::DefaultMainModule::OnEngineStartup();

    m_pCam = EZ_DEFAULT_NEW(Cam, window());

    ezRectFloat levelBounds{
      0,
      0,
      static_cast<float>(m_pWindow->getClientAreaSize().width),
      static_cast<float>(m_pWindow->getClientAreaSize().height)
    };
    levelBounds.x = -0.5f * levelBounds.width;
    levelBounds.y = -0.5f * levelBounds.height;

    level::initialize(kr::move(levelBounds));
  }

  virtual void OnEngineShutdown() override
  {
    level::shutdown();

    EZ_DEFAULT_DELETE(m_pCam);

    kr::DefaultMainModule::OnEngineShutdown();
  }

  virtual void tick() override
  {
    GameLoopData gameLoopData;
    gameLoopData.dt = clock()->GetTimeDiff();
    level::update(gameLoopData);
    if (gameLoopData.stop)
    {
      kr::GlobalGameLoopRegistry::setKeepTicking(false);
    }
  }

} static g_mainModule; // <-- A static instance is mandatory!

