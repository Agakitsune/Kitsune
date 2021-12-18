extern "C" {
#include <X11/Xlib.h>
}
#include <memory>
#include <unordered_map>
#include <fstream>

using namespace std;

class WindowManager {
    public:
        // Whether an existing window manager has been detected. Set by OnWMDetected,
        // and hence must be static.
        static bool wm_detected_;
        // For debug purposes only
        static ofstream log_file;

        ::std::unordered_map<Window, Window> clients_;
        // Factory method for establishing a connection to an X server and creating a
        // WindowManager instance.
        static ::std::unique_ptr<WindowManager> Create();
        // Disconnects from the X server.
        ~WindowManager();
        // The entry point to this class. Enters the main event loop.
        void Run();
        // Xlib error handler. It must be static as its address is passed to Xlib.
        static int OnXError(Display* display, XErrorEvent* e);
        // Xlib error handler used to determine whether another window manager is
        // running. It is set as the error handler right before selecting substructure
        // redirection mask on the root window, so it is invoked if and only if
        // another window manager is running. It must be static as its address is
        // passed to Xlib.
        static int OnWMDetected(Display* display, XErrorEvent* e);

        void Frame(Window window, bool was_created_before_window_manager);
        void Unframe(Window window);

        void OnCreateNotify(const XCreateWindowEvent &e);
        void OnDestroyNotify(const XDestroyWindowEvent &e);
        void OnReparentNotify(const XReparentEvent &e);
        void OnConfigureNotify(const XConfigureEvent &e);
        void OnMapNotify(const XMapEvent &e);
        void OnUnmapNotify(const XUnmapEvent &e);

        void OnConfigureRequest(const XConfigureRequestEvent &e);
        void OnMapRequest(const XMapRequestEvent &e);

        void OnKeyPress(const XKeyEvent &e);

    private:
        // Invoked internally by Create().
        WindowManager(Display* display);

        // Handle to the underlying Xlib Display struct.
        Display* display_;
        // Handle to root window.
        const Window root_;

        const Atom WM_PROTOCOLS;
        const Atom WM_DELETE_WINDOW;
};
