#include <asteroids/pch.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>

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

kr::Owned<kr::Window> createAsteroidsWindow()
{
  ezWindowCreationDesc desc;
  desc.m_ClientAreaSize.width = 600;
  desc.m_ClientAreaSize.height = 480;
  desc.m_Title = "Asteroids";

  return kr::Window::createAndOpen(desc);
}

int main(int argc, char* argv[])
{
  {
    BasicLogging _basicLogging;

    ezStartup::StartupCore();

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    FileLogging _fileLogging;

    mountAsteroidsDataDirs();

    {
      auto window = createAsteroidsWindow();
      bool run = true;
      window->getEvent().AddEventHandler([&run](const kr::WindowEventArgs& e)
      {
        if(e.type == kr::WindowEventArgs::ClickClose)
        {
          run = false;
        }
      });

      auto now = ezTime::Now();
      while(run)
      {
        auto dt = ezTime::Now() - now;

        kr::processWindowMessages(window);
        kr::Renderer::extract();
        kr::Renderer::update(dt, window);

        now += dt;
      }

      ezStartup::StartupEngine();
    }
  }

  ezStartup::ShutdownCore();

  return 0;
}
