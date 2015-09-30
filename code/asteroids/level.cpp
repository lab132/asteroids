#include <asteroids/level.h>

#include <krEngine/rendering/extraction.h>
#include <Core/Input/InputManager.h>

#include <cstdlib>
#include <ctime>
#include <random>

#if !defined(ASTEROIDS_DETERMINISTIC_RANDOM)
  #define ASTEROIDS_DETERMINISTIC_RANDOM EZ_OFF
#endif

using namespace kr;

static ezDynamicArray<Owned<Texture>> g_textures;
static ezDynamicArray<Owned<Sampler>> g_samplers;
static ezDynamicArray<Owned<ShaderProgram>> g_shaders;

#if EZ_ENABLED(ASTEROIDS_DETERMINISTIC_RANDOM)
  static const unsigned int g_randomSeed = 0;
#else
  static const unsigned int g_randomSeed = std::random_device()();
#endif
static auto g_randomEngine = std::default_random_engine(g_randomSeed);

namespace
{
  struct SpacialData
  {
    Transform2D* transform;
    ezVec2* linearVelocity;
    float* boundingRadius;
  };

  struct Ship
  {
    enum { NumLives = 3 };

    Sprite hull;
    Sprite thruster;

    Transform2D transform = Transform2D::zero();
    ezVec2 linearVelocity = ezVec2::ZeroVector();
    float boundingRadius = 0.0f;
    const float speedIncRate = 300.0f; ///< Meters per second per second.
    const float maxSpeed = 300.0f;
    const float linearDamping = 0.9f; ///< Percentual reduction per frame.
    const ezAngle turnSpeed = ezAngle::Degree(360.0f);
    int lives = NumLives;
    ezTime invulnerableTime;

    bool isInvulnerable() const { return this->invulnerableTime > ezTime::Seconds(0); }
  };

  struct Asteroid
  {
    enum { NumLives = 3, MinSpeed = 30, MaxSpeed = 200 };

    Sprite body;

    Transform2D transform = Transform2D::zero();
    ezVec2 linearVelocity = ezVec2::ZeroVector();
    float boundingRadius = 0.0f;

    int lives = NumLives;

    bool isAlive() const { return this->lives > 0; }
    bool operator ==(const Asteroid& rhs) const { return this == &rhs; }
  };

  struct Bullet
  {
    Sprite body;

    Transform2D transform = Transform2D::zero();
    ezVec2 linearVelocity = ezVec2::ZeroVector();
    float boundingRadius = 0.0f;

    const float speed = 500.0f; // Meters per second.
    const ezTime maxLifeTime = ezTime::Seconds(1);
    ezTime lifeTime;

    bool isAlive() const { return this->lifeTime > ezTime(); }
  };
}

static void move(Transform2D& transform, const ezVec2& delta)
{
  transform.position += delta;
}

static void rotate(Transform2D& transform, ezAngle delta)
{
  transform.rotation += delta;
}

static void rotate(ezVec2& vec, ezAngle angle)
{
  auto px = vec.x * ezMath::Cos(angle) - vec.y * ezMath::Sin(angle);
  auto py = vec.x * ezMath::Sin(angle) + vec.y * ezMath::Cos(angle);
  vec.Set(px, py);
}

static void registerInputAction(const char* inputSet, const char* inputAction,
                                const char* szKey1,
                                const char* szKey2 = nullptr,
                                const char* szKey3 = nullptr)
{
  auto cfg = ezInputManager::GetInputActionConfig(inputSet, inputAction);
  cfg.m_bApplyTimeScaling = true;

  if(szKey1 != nullptr) cfg.m_sInputSlotTrigger[0] = szKey1;
  if(szKey2 != nullptr) cfg.m_sInputSlotTrigger[1] = szKey2;
  if(szKey3 != nullptr) cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(inputSet, inputAction, cfg, true);
}

static ezRectFloat g_levelBounds;
static Sprite g_bg;
static Sprite g_life;
static Ship g_ship;
static Bullet g_bullet;
static bool g_drawThruster = false;
static bool g_drawShip = true;
static ezDynamicArray<Asteroid> g_asteroids;

static float leftOf(const ezRectFloat& rect)
{
  return rect.x;
}

static float rightOf(const ezRectFloat& rect)
{
  return rect.x + rect.width;
}

static float bottomOf(const ezRectFloat& rect)
{
  return rect.y;
}

static float topOf(const ezRectFloat& rect)
{
  return rect.y + rect.height;
}

static void extractLevel(Renderer::Extractor& e)
{
  extract(e, g_bg, Transform2D::zero());

  if (g_bullet.body.needsUpdate())
  {
    update(g_bullet.body);
  }

  if (g_bullet.isAlive())
  {
    extract(e, g_bullet.body, g_bullet.transform);
  }

  if (g_ship.isInvulnerable())
  {
    g_drawShip =
      !g_drawShip;
  }

  if (g_drawShip)
  {
    if(g_ship.hull.needsUpdate())
    {
      update(g_ship.hull);
    }
    extract(e, g_ship.hull, g_ship.transform);
    if (ezInputManager::GetInputActionState("game", "thrust") == ezKeyState::Down)
    {
      if (g_ship.isInvulnerable() || g_drawThruster)
      {
        if(g_ship.thruster.needsUpdate())
        {
          update(g_ship.thruster);
        }
        extract(e, g_ship.thruster, g_ship.transform);
      }
    }
  }

  g_drawThruster = !g_drawThruster;

  for (auto& a : g_asteroids)
  {
    if(!a.isAlive())
    {
      continue;
    }

    if (a.body.needsUpdate())
    {
      update(a.body);
    }
    extract(e, a.body, a.transform);
  }

  if (g_life.needsUpdate())
  {
    update(g_life);
  }

  const auto lifeMargin = 8;
  Transform2D livesTransfrom;
  livesTransfrom.rotation = ezAngle::Radian(0);
  livesTransfrom.position = ezVec2(g_levelBounds.x,
                                   g_levelBounds.y + g_levelBounds.height - g_life.getLocalBounds().height);
  livesTransfrom.position.x += lifeMargin;
  livesTransfrom.position.y -= lifeMargin;
  for (int i = 0; i < g_ship.lives; ++i)
  {
    extract(e, g_life, livesTransfrom);
    livesTransfrom.position.x += g_life.getLocalBounds().width + lifeMargin;
  }
}

static ezVec2 randomPos()
{
  std::uniform_real_distribution<float> xdist(g_levelBounds.x, g_levelBounds.width);
  std::uniform_real_distribution<float> ydist(g_levelBounds.y, g_levelBounds.height);

  ezVec2 pos;
  pos.x = xdist(g_randomEngine);
  pos.y = ydist(g_randomEngine);
  return pos;
}

static ezVec2 randomLinearVelocity()
{
  std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
  std::uniform_real_distribution<float> lengthDist(Asteroid::MinSpeed, Asteroid::MaxSpeed);

  auto angle = ezAngle::Degree(angleDist(g_randomEngine));

  ezVec2 dir(0, 1);
  rotate(dir, angle);

  return dir * lengthDist(g_randomEngine);
}

static ezColor randomColor()
{
  std::uniform_real_distribution<float> dist;
  return ezColor{
    dist(g_randomEngine),
    dist(g_randomEngine),
    dist(g_randomEngine),
    1.0f
  };
}

static void center(Sprite& sprite)
{
  auto bounds = sprite.getLocalBounds();
  bounds.x = -0.5f * bounds.width;
  bounds.y = -0.5f * bounds.height;
  sprite.setLocalBounds(move(bounds));
}

static Asteroid& spawnAsteroid()
{
  auto& a = g_asteroids.ExpandAndGetRef();
  initialize(a.body, g_textures[2], g_samplers[0], g_shaders[0]);
  center(a.body);
  a.boundingRadius = 0.5f * a.body.getLocalBounds().width;
  //a.body.setColor(randomColor());
  return a;
}

template<typename T>
static SpacialData spatialData(T& obj)
{
  return SpacialData{ &obj.transform, &obj.linearVelocity, &obj.boundingRadius };
}

static bool areColliding(const SpacialData& a, const SpacialData& b)
{
  auto diff = b.transform->position - a.transform->position;
  return diff.IsZero() || diff.GetLength() < *a.boundingRadius + *b.boundingRadius;
}

static Asteroid& spawnAsteroidRandomized()
{
  auto& a = spawnAsteroid();
  do
  {
    a.transform.position = randomPos();
    // Brute force!
  } while (areColliding(spatialData(g_ship), spatialData(a)));
  a.linearVelocity = randomLinearVelocity();
  return a;
}

enum { NumInitialAsteroids = 3 };

void level::initialize(ezRectFloat levelBounds)
{
  EZ_LOG_BLOCK("Initialize Level");

  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  g_levelBounds = move(levelBounds);

  g_textures.ExpandAndGetRef() = Texture::load("<texture>ship.dds");
  g_textures.ExpandAndGetRef() = Texture::load("<texture>thrust.dds");
  g_textures.ExpandAndGetRef() = Texture::load("<texture>asteroid.dds");
  g_textures.ExpandAndGetRef() = Texture::load("<texture>bullet.dds");
  g_textures.ExpandAndGetRef() = Texture::load("<texture>background.dds");
  g_samplers.ExpandAndGetRef() = Sampler::create();
  g_shaders.ExpandAndGetRef() = ShaderProgram::loadAndLink("<shader>sprite.vs", "<shader>sprite.fs");

  // Background
  // ==========
  initialize(g_bg, g_textures[4], g_samplers[0], g_shaders[0]);
  center(g_bg);
  update(g_bg);

  // Ship And Thruster
  // =================
  auto shipTex = borrow(g_textures[0]);
  initialize(g_ship.hull, shipTex, g_samplers[0], g_shaders[0]);
  center(g_ship.hull);

  auto thrusterTex = borrow(g_textures[1]);
  g_ship.thruster.setLocalBounds(ezRectFloat(-16, -64, 0, 0));
  initialize(g_ship.thruster, thrusterTex, g_samplers[0], g_shaders[0]);

  g_ship.transform.position = ezVec2::ZeroVector();
  g_ship.transform.rotation = ezAngle();
  g_ship.boundingRadius = 0.45f * shipTex->getWidth();

  // Life
  // ====
  g_life.setLocalBounds(ezRectFloat(0,
                                    0,
                                    0.5f * g_ship.hull.getLocalBounds().width,
                                    0.5f * g_ship.hull.getLocalBounds().height));
  initialize(g_life, shipTex, g_samplers[0], g_shaders[0]);

  // Asteroids
  // =========
  for (int i = 0; i < NumInitialAsteroids; ++i)
  {
    spawnAsteroidRandomized();
  }

  // Bullet
  // ======
  auto bulletTex = borrow(g_textures[3]);
  g_bullet.body.setColor(ezColor::LightCyan);
  initialize(g_bullet.body, bulletTex, g_samplers[0], g_shaders[0]);
  center(g_bullet.body);
  g_bullet.boundingRadius = 0.5f * g_bullet.body.getLocalBounds().width;

  Renderer::addExtractionListener(extractLevel);

  // Input
  // =====
  registerInputAction("main", "quit", ezInputSlot_KeyEscape);
  registerInputAction("main", "reset", ezInputSlot_KeyR);

  registerInputAction("game", "thrust", ezInputSlot_KeyW, ezInputSlot_KeyUp);
  registerInputAction("game", "turnCCW_keyboard", ezInputSlot_KeyA, ezInputSlot_KeyLeft);
  registerInputAction("game", "turnCW_keyboard", ezInputSlot_KeyD, ezInputSlot_KeyRight);
  registerInputAction("game", "shoot", ezInputSlot_KeySpace);
}

void level::shutdown()
{
  EZ_LOG_BLOCK("Shutdown Level");

  Renderer::removeExtractionListener(extractLevel);

  g_bg.~Sprite();
  g_life.~Sprite();
  g_ship.~Ship();
  g_bullet.~Bullet();
  g_asteroids.Clear();

  g_shaders.Clear();
  g_samplers.Clear();
  g_textures.Clear();
}

static void updateShipRotation()
{
  float inputValue;
  ezAngle turnDelta;

  if(ezInputManager::GetInputActionState("game", "turnCCW_keyboard", &inputValue) == ezKeyState::Down)
  {
    turnDelta += g_ship.turnSpeed * inputValue;
  }

  if(ezInputManager::GetInputActionState("game", "turnCW_keyboard", &inputValue) == ezKeyState::Down)
  {
    turnDelta -= g_ship.turnSpeed * inputValue;
  }

  rotate(g_ship.transform, turnDelta);
}

static void updateShipMovement(ezTime dt)
{
  float inputValue;
  float thrustValue = 0.0f;

  if(ezInputManager::GetInputActionState("game", "thrust", &inputValue) == ezKeyState::Down)
  {
    thrustValue = g_ship.speedIncRate * inputValue;
  }
  else
  {
    // Apply linear damping.

    g_ship.linearVelocity *= 1.0f - g_ship.linearDamping * static_cast<float>(dt.GetSeconds());
  }

  ezVec2 shipDir(0, 1);
  rotate(shipDir, g_ship.transform.rotation);
  g_ship.linearVelocity += (shipDir * thrustValue);

  if(g_ship.linearVelocity.GetLengthSquared() > ezMath::Square(g_ship.maxSpeed))
  {
    g_ship.linearVelocity.Normalize();
    g_ship.linearVelocity *= g_ship.maxSpeed;
  }

  auto moveDelta = g_ship.linearVelocity * static_cast<float>(dt.GetSeconds());
  move(g_ship.transform, moveDelta);
}

static void levelBoundsCheck(SpacialData& spacial)
{
  auto& pos = spacial.transform->position;
  auto radius = 1.1f * *spacial.boundingRadius;
  if(pos.x < leftOf(g_levelBounds) - radius)   { pos.x = rightOf(g_levelBounds) + radius; }
  if(pos.x > rightOf(g_levelBounds) + radius)  { pos.x = leftOf(g_levelBounds) - radius; }
  if(pos.y < bottomOf(g_levelBounds) - radius) { pos.y = topOf(g_levelBounds) + radius; }
  if(pos.y > topOf(g_levelBounds) + radius)    { pos.y = bottomOf(g_levelBounds) - radius; }
}

static void spawnBullet()
{
  ezVec2 shipDir(0, 1);
  rotate(shipDir, g_ship.transform.rotation);

  g_bullet.transform = g_ship.transform;
  g_bullet.transform.position += shipDir * g_ship.boundingRadius;

  g_bullet.linearVelocity = shipDir * g_bullet.speed;

  g_bullet.lifeTime = g_bullet.maxLifeTime;
}

static void updateBulletMovement(ezTime dt)
{
  auto moveDelta = g_bullet.linearVelocity * static_cast<float>(dt.GetSeconds());
  move(g_bullet.transform, moveDelta);
}

static void updateAsteroidMovements(ezTime dt)
{
  for (auto& a : g_asteroids)
  {
    if(!a.isAlive())
    {
      continue;
    }

    auto moveDelta = a.linearVelocity * static_cast<float>(dt.GetSeconds());
    move(a.transform, moveDelta);
  }
}

static void destroy(Asteroid& asteroid)
{
  --asteroid.lives;

  if (asteroid.lives < 1)
  {
    return;
  }

  const auto shrinkAmount = 16;
  const auto degrees = 45.0f;

  asteroid.boundingRadius -= 0.5f * shrinkAmount;
  auto bounds = asteroid.body.getLocalBounds();
  bounds.width -= shrinkAmount;
  bounds.height -= shrinkAmount;
  asteroid.body.setLocalBounds(bounds);
  center(asteroid.body);

  asteroid.linearVelocity = asteroid.linearVelocity.GetLength() * g_bullet.linearVelocity.GetNormalized();
  rotate(asteroid.linearVelocity, ezAngle::Degree(degrees));

  auto& other = spawnAsteroid();
  other = asteroid;
  rotate(other.linearVelocity, ezAngle::Degree(-2.0f * degrees));
}

void level::update(GameLoopData& gameLoop)
{
  ezInputManager::Update(gameLoop.dt);

  if (ezInputManager::GetInputActionState("main", "quit") == ezKeyState::Pressed)
  {
    gameLoop.stop = true;
    return;
  }

  int numLiveAsteroids = 0;
  for (auto& a : g_asteroids)
  {
    if (a.isAlive())
    {
      ++numLiveAsteroids;
    }
  }

  if(numLiveAsteroids == 0)
  {
    ezLog::Success("You destroyed all asteroids!");
    ezLog::Success("Game Over");
    gameLoop.stop = true;
    return;
  }

  if(ezInputManager::GetInputActionState("main", "reset") == ezKeyState::Pressed)
  {
    g_ship.transform = Transform2D::zero();
    g_ship.linearVelocity.SetZero();

    g_bullet.lifeTime = ezTime::Seconds(0.0f);

    g_asteroids.Clear();
    for (int i = 0; i < NumInitialAsteroids; ++i)
    {
      spawnAsteroidRandomized();
    }
  }

  if (g_bullet.isAlive())
  {
    g_bullet.lifeTime -= gameLoop.dt;
  }

  if (ezInputManager::GetInputActionState("game", "shoot") == ezKeyState::Down
      && !g_bullet.isAlive()
      && !g_ship.isInvulnerable())
  {
    spawnBullet();
  }

  // Update Movement/Rotation
  // ========================

  // Ship
  updateShipRotation();
  updateShipMovement(gameLoop.dt);

  // Bullet
  updateBulletMovement(gameLoop.dt);

  // Asteroids
  updateAsteroidMovements(gameLoop.dt);

  // Level Bounds Checking
  // =====================

  // Ship
  levelBoundsCheck(spatialData(g_ship));

  // Bullet
  levelBoundsCheck(spatialData(g_bullet));

  // Asteroids
  for (auto& a : g_asteroids)
  {
    if (!a.isAlive())
    {
      continue;
    }

    levelBoundsCheck(spatialData(a));
  }

  // Collision With Bullet
  // =====================
  if (g_bullet.isAlive())
  {
    auto bulletSpacial = spatialData(g_bullet);
    for (auto& a : g_asteroids)
    {
      if(!a.isAlive())
      {
        continue;
      }

      if (areColliding(bulletSpacial, spatialData(a)))
      {
        destroy(a);
        g_bullet.lifeTime = ezTime::Seconds(0);
        break;
      }
    }
  }

  // Collision With Ship
  // ===================
  if (g_ship.isInvulnerable())
  {
    g_ship.invulnerableTime -= gameLoop.dt;

    if (!g_ship.isInvulnerable())
    {
      g_drawShip = true;
    }
  }
  else
  {
    auto shipSpacial = spatialData(g_ship);
    for(auto& a : g_asteroids)
    {
      if(!a.isAlive())
      {
        continue;
      }

      if(areColliding(shipSpacial, spatialData(a)))
      {
        ezLog::Info("You ship was hit!");
        --g_ship.lives;
        g_ship.invulnerableTime = ezTime::Seconds(2);
        ezLog::Info("Remaining lives: %d", g_ship.lives);
      }
    }
  }

  if (g_ship.lives < 1)
  {
    ezLog::Info("Your ship was destroyed.");
    ezLog::Info("Game Over");
    gameLoop.stop = true;
    return;
  }
}
