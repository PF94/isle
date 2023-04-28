#include "isle.h"

#include "define.h"
#include "../lib/legoomni.h"
#include "../lib/mxdirectdraw.h"
#include "../lib/mxdsaction.h"
#include "../lib/mxomni.h"

RECT windowRect = {0, 0, 640, 480};

Isle::Isle()
{
  m_hdPath = NULL;
  m_cdPath = NULL;
  m_deviceId = NULL;
  m_savePath = NULL;
  m_fullScreen = 1;
  m_flipSurfaces = 0;
  m_backBuffersInVram = 1;
  m_using8bit = 0;
  m_using16bit = 1;
  m_unk24 = 0;
  m_drawCursor = 0;
  m_use3dSound = 1;
  m_useMusic = 1;
  m_useJoystick = 0;
  m_joystickIndex = 0;
  m_wideViewAngle = 1;
  m_islandQuality = 1;
  m_islandTexture = 1;
  m_gameStarted = 0;
  m_frameDelta = 10;
  m_windowActive = 1;

  MxRect32 rect;
  rect.m_left = 0;
  rect.m_top = 0;
  rect.m_right = 639;
  rect.m_bottom = 479;

  m_videoParam = MxVideoParam(rect, NULL, 1, MxVideoParamFlags());
  m_videoParam.flags().Enable16Bit(MxDirectDraw::GetPrimaryBitDepth() == 16);

  m_windowHandle = NULL;
  m_cursor1 = NULL;
  m_cursor2 = NULL;
  m_cursor3 = NULL;
  m_cursor4 = NULL;

  LegoOmni::CreateInstance();
}

Isle::~Isle()
{
  if (LegoOmni::GetInstance()) {
    close();
    MxOmni::DestroyInstance();
  }

  if (m_hdPath) {
    delete [] m_hdPath;
  }

  if (m_cdPath) {
    delete [] m_cdPath;
  }

  if (m_deviceId) {
    delete [] m_deviceId;
  }

  if (m_savePath) {
    delete [] m_savePath;
  }
}

void Isle::close()
{
  MxDSAction ds;

  if (Lego()) {
    GameState()->Save(0);
    if (InputManager()) {
      InputManager()->QueueEvent(KEYDOWN, 0, 0, 0, 0x20);
    }

    // FIXME: Untangle
    //VideoManager()->GetViewManager()->RemoveAll(NULL);
    //ViewManager::RemoveAll
    //          (*(ViewManager **)(*(int *)(*(int *)(pLVar4 + 0x68) + 8) + 0x88), NULL);

    MxAtomId id;
    long local_88 = 0;
    Lego()->RemoveWorld(id, local_88);
    Lego()->vtable24(ds);
    TransitionManager()->SetWaitIndicator(NULL);
    Lego()->vtable3c();

    long lVar8;
    do {
      lVar8 = Streamer()->Close(NULL);
    } while (lVar8 == 0);

    while (Lego()) {
      if (Lego()->vtable28(ds) != 0) {
        break;
      }

      Timer()->GetRealTime();
      TickleManager()->vtable08();
    }
  }
}

BOOL readReg(LPCSTR name, LPSTR outValue, DWORD outSize)
{
  HKEY hKey;
  DWORD valueType;

  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Mindscape\\LEGO Island", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    if (RegQueryValueExA(hKey, name, NULL, &valueType, (LPBYTE) outValue, &outSize) == ERROR_SUCCESS) {
      if (RegCloseKey(hKey) == ERROR_SUCCESS) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

int readRegBool(LPCSTR name, BOOL *out)
{
  char buffer[256];

  BOOL read = readReg(name, buffer, 0x100);
  if (read) {
    if (strcmp("YES", buffer) == 0) {
      *out = TRUE;
      return TRUE;
    }

    if (strcmp("NO", buffer) == 0) {
      *out = FALSE;
      return TRUE;
    }
  }
  return FALSE;
}

int readRegInt(LPCSTR name, int *out)
{
  char buffer[256];

  if (readReg(name, buffer, 0x100)) {
    *out = atoi(buffer);
    return TRUE;
  }

  return FALSE;
}

void Isle::loadConfig()
{
  char buffer[256];

  if (!readReg("diskpath", buffer, 0x400)) {
    strcpy(buffer, MxOmni::GetHD());
  }

  m_hdPath = new char[strlen(buffer) + 1];
  strcpy(m_hdPath, buffer);
  MxOmni::SetHD(m_hdPath);

  if (!readReg("cdpath", buffer, 0x400)) {
    strcpy(buffer, MxOmni::GetCD());
  }

  m_cdPath = new char[strlen(buffer) + 1];
  strcpy(m_cdPath, buffer);
  MxOmni::SetCD(m_cdPath);

  readRegBool("Flip Surfaces", &m_flipSurfaces);
  readRegBool("Full Screen", &m_fullScreen);
  readRegBool("Wide View Angle", &m_wideViewAngle);
  readRegBool("3DSound", &m_use3dSound);
  readRegBool("Music", &m_useMusic);
  readRegBool("UseJoystick", &m_useJoystick);
  readRegInt("JoystickIndex", &m_joystickIndex);
  readRegBool("Draw Cursor", &m_drawCursor);

  int backBuffersInVRAM;
  if (readRegBool("Back Buffers in Video RAM",&backBuffersInVRAM)) {
    m_backBuffersInVram = !backBuffersInVRAM;
  }

  int bitDepth;
  if (readRegInt("Display Bit Depth", &bitDepth)) {
    if (bitDepth == 8) {
      m_using8bit = TRUE;
    } else if (bitDepth == 16) {
      m_using16bit = TRUE;
    }
  }

  if (!readReg("Island Quality", buffer, 0x400)) {
    strcpy(buffer, "1");
  }
  m_islandQuality = atoi(buffer);

  if (!readReg("Island Texture", buffer, 0x400)) {
    strcpy(buffer, "1");
  }
  m_islandTexture = atoi(buffer);

  if (readReg("3D Device ID", buffer, 0x400)) {
    m_deviceId = new char[strlen(buffer) + 1];
    strcpy(m_deviceId, buffer);
  }

  if (readReg("savepath", buffer, 0x400)) {
    m_savePath = new char[strlen(buffer) + 1];
    strcpy(m_savePath, buffer);
  }
}

void Isle::setupVideoFlags(BOOL fullScreen, BOOL flipSurfaces, BOOL backBuffers,
                           BOOL using8bit, BOOL m_using16bit, BOOL param_6, BOOL param_7,
                           BOOL wideViewAngle, char *deviceId)
{
  m_videoParam.flags().EnableFullScreen(fullScreen);
  m_videoParam.flags().EnableFlipSurfaces(flipSurfaces);
  m_videoParam.flags().EnableBackBuffers(backBuffers);
  m_videoParam.flags().EnableUnknown1(param_6);
  m_videoParam.flags().EnableUnknown2(TRUE);
  m_videoParam.flags().EnableWideViewAngle(wideViewAngle);
  m_videoParam.SetDeviceName(deviceId);
  if (using8bit) {
    m_videoParam.flags().Enable16Bit(FALSE);
  }
  if (m_using16bit) {
    m_videoParam.flags().Enable16Bit(TRUE);
  }
}

BOOL Isle::setupMediaPath()
{
  char mediaPath[256];
  GetProfileStringA("LEGO Island", "MediaPath", "", mediaPath, 0x100);

  MxOmniCreateParam createParam(mediaPath, (struct HWND__ *) m_windowHandle, m_videoParam, MxOmniCreateFlags());

  if (Lego()->Create(createParam) != FAILURE) {
    VariableTable()->SetVariable("ACTOR_01", "");
    TickleManager()->vtable1c(VideoManager(), 10);
    return TRUE;
  }

  return FALSE;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (!g_isle) {
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
  }

  switch (uMsg) {
  case WM_PAINT:
    return DefWindowProcA(hWnd, WM_PAINT, wParam, lParam);
  case WM_ACTIVATE:
    return DefWindowProcA(hWnd, WM_ACTIVATE, wParam, lParam);
  case WM_ACTIVATEAPP:
    if (g_isle) {
      if ((wParam != 0) && (g_isle->m_fullScreen)) {
        MoveWindow(hWnd, windowRect.left, windowRect.top,
                   (windowRect.right - windowRect.left) + 1,
                   (windowRect.bottom - windowRect.top) + 1, TRUE);
      }
      // FIXME: Untangle
      //g_isle->m_windowActive = wParam;
    }
    return DefWindowProcA(hWnd,WM_ACTIVATEAPP,wParam,lParam);
  case WM_CLOSE:
    if (!g_closed && g_isle) {
      if (g_isle) {
        delete g_isle;
      }
      g_isle = NULL;
      g_closed = TRUE;
      return 0;
    }
    return DefWindowProcA(hWnd,WM_CLOSE,wParam,lParam);
  case WM_GETMINMAXINFO:
  {
    MINMAXINFO *mmi = (MINMAXINFO *) lParam;

    mmi->ptMaxTrackSize.x = (windowRect.right - windowRect.left) + 1;
    mmi->ptMaxTrackSize.y = (windowRect.bottom - windowRect.top) + 1;
    mmi->ptMinTrackSize.x = (windowRect.right - windowRect.left) + 1;
    mmi->ptMinTrackSize.y = (windowRect.bottom - windowRect.top) + 1;

    return 0;
  }
  case WM_ENTERMENULOOP:
    return DefWindowProcA(hWnd,WM_ENTERMENULOOP,wParam,lParam);
  case WM_SYSCOMMAND:
    if (wParam == 0xf140) {
      return 0;
    }
    if (wParam == 0xf060 && g_closed == 0) {
      if (g_isle) {
        if (_DAT_00410050 != 0) {
          ShowWindow(g_isle->m_windowHandle, SW_RESTORE);
        }
        PostMessageA(g_isle->m_windowHandle, 0x10, 0, 0);
        return 0;
      }
    } else if (g_isle && g_isle->m_fullScreen && (wParam == 0xf010 || wParam == 0xf100)) {
      return 0;
    }
    return DefWindowProcA(hWnd,WM_SYSCOMMAND,wParam,lParam);
  case WM_EXITMENULOOP:
    return DefWindowProcA(hWnd,WM_EXITMENULOOP,wParam,lParam);
  case WM_MOVING:
    if (g_isle && g_isle->m_fullScreen) {
      GetWindowRect(hWnd, (LPRECT) lParam);
      return 0;
    }
    return DefWindowProcA(hWnd,WM_MOVING,wParam,lParam);
  case WM_NCPAINT:
    if (g_isle && g_isle->m_fullScreen) {
      return 0;
    }
    return DefWindowProcA(hWnd, WM_NCPAINT, wParam, lParam);
  case WM_DISPLAYCHANGE:
    /* FIXME: Untangle
    if (g_isle && VideoManager() && g_isle->m_fullScreen && ((pLVar7 = VideoManager(), *(int *)(pLVar7 + 0x74) != 0 && (pLVar7 = VideoManager(), *(int *)(*(int *)(pLVar7 + 0x74) + 0x880) != 0))) {
      if (_DAT_00410054 == 0) {
        unsigned char bVar1 = FALSE;
        if (LOWORD(lParam) == _DAT_00410058 && HIWORD(lParam) == _DAT_0041005c && _DAT_00410060 == wParam) {
          bVar1 = TRUE;
        }
        if (_DAT_00410050 == 0) {
          if (!bVar1) {
            _DAT_00410050 = 1;
            Lego()->vtable38();
            VideoManager()->DisableRMDevice();
          }
        }
        else if (bVar1) {
          _DAT_00410064 = 1;
        }
      }
      else {
        _DAT_00410054 = 0;
        _DAT_00410060 = wParam;
      }
    }*/
    return DefWindowProcA(hWnd,WM_DISPLAYCHANGE,wParam,lParam);

  case WM_SETCURSOR:
  case WM_KEYDOWN:
  case WM_MOUSEMOVE:
  case WM_TIMER:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  {

    NotificationId type = NONE;
    unsigned char keyCode = 0;

    switch (uMsg) {
    case WM_KEYDOWN:
      if (lParam & 0x40000000) {
        return DefWindowProcA(hWnd,WM_KEYDOWN,wParam,lParam);
      }
      keyCode = wParam;
      type = KEYDOWN;
      break;
    case WM_MOUSEMOVE:
      g_mousemoved = 1;
      type = MOUSEMOVE;
      break;
    case WM_TIMER:
      type = TIMER;
      break;
    case WM_SETCURSOR:
      if (g_isle) {
        HCURSOR hCursor = g_isle->m_cursor4;
        if (g_isle->m_cursor2 == hCursor || g_isle->m_cursor3 == hCursor || hCursor == NULL) {
          SetCursor(hCursor);
          return 0;
        }
      }
      break;
    case WM_LBUTTONDOWN:
      g_mousedown = 1;
      type = MOUSEDOWN;
      break;
    case WM_LBUTTONUP:
      g_mousedown = 0;
      type = MOUSEUP;
      break;
    case 0x5400:
      if (g_isle) {
        // FIXME: Untangle
        //FUN_00402e80(g_isle,wParam);
        return 0;
      }
    }

    if (g_isle) {
      if (InputManager()) {
        InputManager()->QueueEvent(type, wParam, LOWORD(lParam), HIWORD(lParam), keyCode);
      }
      if (g_isle && g_isle->m_drawCursor && type == MOUSEMOVE) {
        unsigned short x = LOWORD(lParam);
        unsigned short y = HIWORD(lParam);
        if (639 < x) {
          x = 639;
        }
        if (479 < y) {
          y = 479;
        }
        VideoManager()->MoveCursor(x,y);
      }
    }
    return 0;
  }
  }

  return DefWindowProcA(hWnd,uMsg,wParam,lParam);
}

MxResult Isle::setupWindow(HINSTANCE hInstance)
{
  WNDCLASSA wndclass;
  ZeroMemory(&wndclass, sizeof(WNDCLASSA));

  loadConfig();

  setupVideoFlags(m_fullScreen, m_flipSurfaces, m_backBuffersInVram, m_using8bit,
                  m_using16bit, m_unk24, FALSE, m_wideViewAngle, m_deviceId);

  MxOmni::SetSound3D(m_use3dSound);

  srand(timeGetTime() / 1000);
  SystemParametersInfoA(SPI_SETMOUSETRAILS, 0, NULL, 0);

  ZeroMemory(&wndclass, sizeof(WNDCLASSA));

  wndclass.cbClsExtra = 0;
  wndclass.style = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc = WndProc;
  wndclass.cbWndExtra = 0;
  wndclass.hIcon = LoadIconA(hInstance, MAKEINTRESOURCE(105));
  wndclass.hCursor = LoadCursorA(hInstance, MAKEINTRESOURCE(102));
  m_cursor4 = wndclass.hCursor;
  m_cursor1 = wndclass.hCursor;
  m_cursor2 = LoadCursorA(hInstance, MAKEINTRESOURCE(104));
  m_cursor3 = LoadCursorA(hInstance, MAKEINTRESOURCE(103));
  wndclass.hInstance = hInstance;
  wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
  wndclass.lpszClassName = WNDCLASS_NAME;

  if (!RegisterClassA(&wndclass)) {
    return FAILURE;
  }

  DWORD dwStyle;
  int x, y, width, height;

  if (!m_fullScreen) {
    AdjustWindowRectEx(&windowRect, WS_CAPTION | WS_SYSMENU, 0, WS_EX_APPWINDOW);

    height = windowRect.bottom - windowRect.top;
    width = windowRect.right - windowRect.left;

    y = CW_USEDEFAULT;
    x = CW_USEDEFAULT;
    dwStyle = WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
  } else {
    AdjustWindowRectEx(&windowRect, WS_CAPTION | WS_SYSMENU, 0, WS_EX_APPWINDOW);
    height = windowRect.bottom - windowRect.top;
    width = windowRect.right - windowRect.left;
    dwStyle = WS_CAPTION | WS_SYSMENU;
    x = windowRect.left;
    y = windowRect.top;
  }

  m_windowHandle = CreateWindowExA(WS_EX_APPWINDOW, WNDCLASS_NAME, WINDOW_TITLE, dwStyle,
                           x, y, width + 1, height + 1, NULL, NULL, hInstance, NULL);
  if (!m_windowHandle) {
    return FAILURE;
  }

  if (m_fullScreen) {
    MoveWindow(m_windowHandle, windowRect.left, windowRect.top, (windowRect.right - windowRect.left) + 1, (windowRect.bottom - windowRect.top) + 1, TRUE);
  }

  ShowWindow(m_windowHandle, SW_SHOWNORMAL);
  UpdateWindow(m_windowHandle);
  if (!setupMediaPath()) {
    return FAILURE;
  }

  GameState()->SetSavePath(m_savePath);
  GameState()->SerializePlayersInfo(1);
  GameState()->SerializeScoreHistory(1);

  int iVar10;
  if (m_islandQuality == 0) {
    iVar10 = 1;
  } else if (m_islandQuality == 1) {
    iVar10 = 2;
  } else {
    iVar10 = 100;
  }

  int uVar1 = (m_islandTexture == 0);
  LegoModelPresenter::configureLegoModelPresenter(uVar1);
  LegoPartPresenter::configureLegoPartPresenter(uVar1,iVar10);
  LegoWorldPresenter::configureLegoWorldPresenter(m_islandQuality);
  LegoBuildingManager::configureLegoBuildingManager(m_islandQuality);
  LegoROI::configureLegoROI(iVar10);
  LegoAnimationManager::configureLegoAnimationManager(m_islandQuality);
  if (LegoOmni::GetInstance()) {
    if (LegoOmni::GetInstance()->GetInputManager()) {
      LegoOmni::GetInstance()->GetInputManager()->m_unk00[0xCD] = m_useJoystick;
      LegoOmni::GetInstance()->GetInputManager()->m_unk00[0x67] = m_joystickIndex;
    }
  }
  if (m_fullScreen) {
    MoveWindow(m_windowHandle, windowRect.left, windowRect.top, (windowRect.right - windowRect.left) + 1, (windowRect.bottom - windowRect.top) + 1, TRUE);
  }
  ShowWindow(m_windowHandle, SW_SHOWNORMAL);
  UpdateWindow(m_windowHandle);

  return SUCCESS;
}

void Isle::tick(BOOL sleepIfNotNextFrame)
{
  if (this->m_windowActive) {
    if (!Lego()) return;
    if (!TickleManager()) return;
    if (!Timer()) return;

    long currentTime = Timer()->GetRealTime();
    if (currentTime < _last_frame_time) {
      _last_frame_time = -this->m_frameDelta;
    }
    if (this->m_frameDelta + _last_frame_time < currentTime) {
      if (!Lego()->vtable40()) {
        TickleManager()->vtable08();
      }
      _last_frame_time = currentTime;

      if (_DAT_004101bc == 0) {
        return;
      }

      _DAT_004101bc--;
      if (_DAT_004101bc != 0) {
        return;
      }

      LegoOmni::GetInstance()->CreateBackgroundAudio();
      BackgroundAudioManager()->Enable(this->m_useMusic);

      MxStreamController *stream = Streamer()->Open("\\lego\\scripts\\isle\\isle", 0);
      MxDSAction ds;

      if (!stream) {
        stream = Streamer()->Open("\\lego\\scripts\\nocd", 0);
        if (!stream) {
          return;
        }

        ds.m_atomId = stream->atom;
        ds.m_unk24 = 0xFFFF;
        ds.m_unk1c = 0;
        VideoManager()->EnableFullScreenMovie(TRUE, TRUE);

        if (Start(&ds) != SUCCESS) {
          return;
        }
      } else {
        ds.m_atomId = stream->atom;
        ds.m_unk24 = 0xFFFF;
        ds.m_unk1c = 0;
        if (Start(&ds) != SUCCESS) {
          return;
        }
        this->m_gameStarted = 1;
      }
      return;
    }
    if (sleepIfNotNextFrame == 0) return;
  }

  Sleep(0);
}