extern "C" {
    #include <X11/Xutil.h>
}

#include "test.hpp"
#include <glog/logging.h>
#include <iostream>
#include <fstream>

using ::std::unique_ptr;
using namespace std;

bool WindowManager::wm_detected_ = false;
ofstream WindowManager::log_file;

unique_ptr<WindowManager> WindowManager::Create() {
    // 1. Open X display
    log_file.open("latest_log.txt", ios::out | ios::trunc);

    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        LOG(ERROR) << "Failed to open X display " << XDisplayName(NULL);
        return NULL;
    }
    // 2. Construct WindowManager instance.
    return unique_ptr<WindowManager>(new WindowManager(display));
}

WindowManager::WindowManager(Display* display)
    : display_(CHECK_NOTNULL(display)),
      root_(DefaultRootWindow(display_)),
      WM_PROTOCOLS(XInternAtom(display_, "WM_PROTOCOLS", false)),
      WM_DELETE_WINDOW(XInternAtom(display_, "WM_DELETE_WINDOW", false)) {
}

WindowManager::~WindowManager() {
    log_file.close();
    XCloseDisplay(display_);
}

void WindowManager::Run() {
    //1. Initialization
    wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(display_, root_, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display_, false);
    if (wm_detected_) {
        LOG(ERROR) << "Detected another window manager on display" << XDisplayString(display_);
        return;
    }
    XSetErrorHandler(&WindowManager::OnXError);
    XGrabServer(display_);
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    CHECK(XQueryTree(display_, root_, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows));
    CHECK_EQ(returned_root, root_);
    for (unsigned int i = 0; i < num_top_level_windows; i++) {
        Frame(top_level_windows[i], true);
    }
    XFree(top_level_windows);
    XUngrabServer(display_);
    log_file << "Bruh\n";
    //2. Main event loop
    for (;;) {
        log_file << "Fun\n";
        XEvent e;
        log_file << "Hub\n";
        log_file << XNextEvent(display_, &e);
        log_file << "Received event: " << e.type << "\n";

        switch (e.type) {
            case CreateNotify:
                OnCreateNotify(e.xcreatewindow);
                break;
            case DestroyNotify:
                OnDestroyNotify(e.xdestroywindow);
                break;
            case ReparentNotify:
                OnReparentNotify(e.xreparent);
                break;
            case ConfigureNotify:
                OnConfigureNotify(e.xconfigure);
                break;
            case MapNotify:
                OnMapNotify(e.xmap);
                break;
            case UnmapNotify:
                OnMapRequest(e.xmaprequest);
                break;
            case ConfigureRequest:
                OnConfigureRequest(e.xconfigurerequest);
                break;
            case MapRequest:
                OnMapRequest(e.xmaprequest);
                break;
            case KeyPress:
                OnKeyPress(e.xkey);
                break;
            default:
                LOG(WARNING) << "Ignored event";
        }
    }
    log_file << "Duh\n";
}

int WindowManager::OnWMDetected(Display *display, XErrorEvent *e) {
    CHECK_EQ(static_cast<int>(e->error_code), BadAccess);
    wm_detected_ = true;
    return 0;
}

int WindowManager::OnXError(Display *display, XErrorEvent *e) {
    CHECK_EQ(static_cast<int>(e->error_code), BadAccess);
    LOG(ERROR) << "BAD ACCESS";
    return 0;
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent &e) {
    LOG(INFO) << "Window Created";
}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent &e) {
    LOG(INFO) << "Window Destroyed";
}

void WindowManager::OnReparentNotify(const XReparentEvent &e) {
    LOG(INFO) << "Window Reparented";
}

void WindowManager::OnConfigureNotify(const XConfigureEvent &e) {
    LOG(INFO) << "Window Configured";
}

void WindowManager::OnMapNotify(const XMapEvent &e) {
    LOG(INFO) << "Window Mapped";
}

void WindowManager::OnUnmapNotify(const XUnmapEvent &e) {
    if (clients_.count(e.window)) {
        LOG(INFO) << "Ignore UnmapNotify for non-client window " << e.window;
        return;
    }
    if (e.event == root_) {
        LOG(INFO) << "Ignore UnmapNotify for reparented pre-existing window " << e.window;
        return;
    }
    Unframe(e.window);
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent &e) {
    XWindowChanges changes;

    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    if (clients_.count(e.window)) {
        const Window frame = clients_[e.window];
        XConfigureWindow(display_, frame, e.value_mask, &changes);
        LOG(INFO) << "Resize [" << frame << "] to ";
    }
}

void WindowManager::OnMapRequest(const XMapRequestEvent &e) {
    Frame(e.window, false);
    XMapWindow(display_, e.window);
}

void WindowManager::OnKeyPress(const XKeyEvent &e) {
    log_file << "Key Pressed\n";
}

void WindowManager::Frame(Window window, bool was_created_before_window_manager) {
    const unsigned int BORDER_WIDTH = 3;
    const unsigned long BORDER_COLOR = 0xff0000;
    const unsigned long BG_COLOR = 0x0000ff;

    XWindowAttributes x_window_attrs;
    CHECK(XGetWindowAttributes(display_, window, &x_window_attrs));

    if (was_created_before_window_manager) {
        if (x_window_attrs.override_redirect || x_window_attrs.map_state != IsViewable) {
            return;
        }
    }
    const Window frame = XCreateSimpleWindow(display_, root_, x_window_attrs.x, x_window_attrs.y, x_window_attrs.width, x_window_attrs.height, BORDER_WIDTH, BORDER_COLOR, BG_COLOR);
    XSelectInput(display_, frame, SubstructureRedirectMask | SubstructureNotifyMask);
    XAddToSaveSet(display_, window);
    XReparentWindow(display_, window, frame, 0, 0);
    XMapWindow(display_, frame);
    clients_[window] = frame;
    LOG(INFO) << "Framed window " << window << " [" << frame << "]";
}

void WindowManager::Unframe(Window window) {
    const Window frame = clients_[window];
    
    XUnmapWindow(display_, frame);
    XReparentWindow(display_, window, root_, 0, 0);
    XRemoveFromSaveSet(display_, window);
    XDestroyWindow(display_, frame);
    clients_.erase(window);
    LOG(INFO) << "Unframed window " << window << " [" << frame << "]";
}
