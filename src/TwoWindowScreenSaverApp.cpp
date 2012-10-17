#include "cinder/Cinder.h"
#include "cinder/app/AppScreenSaver.h"
#include "cinder/Display.h"
#include "cinder/Camera.h"
#include "cinder/Color.h"

#pragma comment(lib, "comctl32.lib")
// Link to the correct library based on the unicode setting.
#ifdef UNICODE
#pragma comment(lib, "ScrnSavw.lib")
#else
#pragma comment(lib, "ScrnSave.lib")
#endif

using namespace std;
using namespace ci;
using namespace ci::app;

class TwoWindowScreenSaverApp : public AppScreenSaver {
public:
  virtual void setup();
  virtual void update();
  virtual void draw();
  virtual void resize( ResizeEvent event );

protected:
  ci::Color	mColor, mBackgroundColor;
  float		mRadius;
  bool mHasTwoDisplays;
  bool mIsFirstResize;
  ci::Area mMainArea;
  ci::Area mSecondArea;
};


void TwoWindowScreenSaverApp::setup()
{
  mColor = Color( 1.0f, 0.5f, 0.25f );
  mBackgroundColor = Color( 0.25f, 0.0f, 0.0f );

  mIsFirstResize = true; // Need a flag for handling the first resize
  mHasTwoDisplays = false; // Need a flag for determining if we've got two displays

  if(ci::Display::getDisplays().size() > 1) {
    mHasTwoDisplays = true;
  }
}

void TwoWindowScreenSaverApp::resize( ResizeEvent event )
{
  // When we resize for the first time, we'll determine if we need
  // to pop up that second window on the secondary display
  if(mIsFirstResize) {
    mIsFirstResize = false;

    // Let's make sure that if we've got two displays, we want to make sure the window size isn't smaller
    // than the main display size, because if that's the case, then we're in the screen saver Preview.
    if(mHasTwoDisplays) {
      if(ci::Display::getMainDisplay()->getWidth() > event.getWidth()) {
        mHasTwoDisplays = false;
      }
    }

    ci::Area mainDisplayArea = ci::Display::getMainDisplay()->getArea();

    if(mHasTwoDisplays) {

      // First, we stick our primary window over into the main display area.
      ::SetWindowPos( ((RendererGl *)getRenderer())->getHwnd(), 
        HWND_TOPMOST, 
        mainDisplayArea.getX1(), mainDisplayArea.getY1(),
        mainDisplayArea.getWidth(), mainDisplayArea.getHeight(),
        SWP_SHOWWINDOW);

      // Need to resize the GL viewport to accomodate the new size.
      glViewport( 0, 0, mainDisplayArea.getWidth(), mainDisplayArea.getHeight());
      cinder::CameraPersp cam( mainDisplayArea.getWidth(), mainDisplayArea.getHeight(), 60.0f );
      glMatrixMode( GL_PROJECTION );
      glLoadMatrixf( cam.getProjectionMatrix().m );

      glMatrixMode( GL_MODELVIEW );
      glLoadMatrixf( cam.getModelViewMatrix().m );
      glScalef( 1.0f, -1.0f, 1.0f ); // invert Y axis so increasing Y goes down.
      glTranslatef( 0.0f, (float) - mainDisplayArea.getHeight(), 0.0f ); // shift origin up to upper-left corner.
      // Done with the new viewport and translation stuff.

      mainDisplayArea = ci::Display::getMainDisplay()->getArea();
      mMainArea = mainDisplayArea;

      // Now that we know we've got two displays, we need to create the second window in the other display.
      // First, we get the two displays' areas and window area so that we can determine
      // proper mapping.
      int displayNum = 0;
      ci::Area secondArea;

      std::vector<DisplayRef> displays = ci::Display::getDisplays();
      std::vector<DisplayRef>::iterator it;
      // Let's loop thru them and see what their offsets are.
      for(it = displays.begin(); it != displays.end(); it++) {
        if(displayNum != 0) {
          secondArea = (*it)->getArea();
        }
        displayNum++;
      }

      // Set up the new Window struct
      WNDCLASS scl;
      LPCWSTR szCWinName = TEXT("Screen Saver Second Screen");
      LPCWSTR szCClassName = TEXT("SCRNSAVESECOND");
      HINSTANCE instance = ::GetModuleHandle( NULL );

      if(!instance) {
        console() << "No HINSTANCE exists." << endl;
      }

      scl.hInstance = instance;
      scl.lpszClassName = szCClassName;
      scl.lpfnWndProc = DefWindowProc;
      scl.style = 0;
      scl.hIcon = ::LoadIcon(NULL, IDI_WINLOGO);
      scl.hCursor = ::LoadCursor(NULL, IDC_ARROW);
      scl.lpszMenuName = NULL;
      scl.cbClsExtra = 0;
      scl.cbWndExtra = 0;
      scl.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // Making the background black

      if(! RegisterClass(&scl)) {
        console() << "There was an error registering our new window class" << endl;
      }

      HWND secondWnd = CreateWindow(
        szCClassName,
        szCWinName,
        WS_POPUP|WS_CHILD,
        secondArea.x1,  // x location
        secondArea.x2,  // y location
        secondArea.getWidth(),  // width
        secondArea.getHeight(),  // height
        ((RendererGl *)getRenderer())->getHwnd(),
        NULL,
        instance,
        NULL
        );

      if(! secondWnd) {
        console() << "Error: No window was created." << endl;
      }

      // Show and position the newly created window.
      ::ShowWindow( secondWnd, SW_SHOW );
      ::UpdateWindow( secondWnd );
      ::SetWindowPos( secondWnd, 
        HWND_TOPMOST, 
        secondArea.getX1(), secondArea.getY1(),
        secondArea.getWidth(), secondArea.getHeight(),
        SWP_SHOWWINDOW);

    } else {

      // Not doing two displays, so we want to count on the normal
      // resize event's size that happens to determine our drawing area.
      mMainArea.set(0, 0, event.getWidth(), event.getHeight());

      // At this point, we can also elect to figure out if we're dealing with
      // the preview display area. Because if the main display size is greater
      // than the event's width / height, then we know we're dealing with 
      // the preview mode.

    } // End check for two displays.

  } // End check if first resize.

}

void TwoWindowScreenSaverApp::update()
{
  mRadius = abs( cos( getElapsedSeconds() ) * 200 );
}

void TwoWindowScreenSaverApp::draw()
{
  gl::clear( mBackgroundColor );
  glColor3f( mColor );

  // General getWindowCenter() doesn't work anymore because that
  // calculation is done before our resize stuff happens. This is
  // why we keep track of mMainArea.

  gl::drawSolidCircle( ci::Vec2f( mMainArea.getWidth() / 2.f, mMainArea.getHeight() / 2.f ), mRadius );
}


CINDER_APP_SCREENSAVER( TwoWindowScreenSaverApp, RendererGl )
