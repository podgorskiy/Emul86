/*
#include "GLFWCallbacks.h"
#include "_assert.h"

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#ifdef SYGLASS_FREE
#include "ConfigSettings/ConfigManager.h"
#include "Foundation/SWCManager/SWCManager.h"
#include "Projects/BaseProject.h"
#include "TaskManager/TaskManager.h"
#endif

void GLFWCallbacks::ErrorCallback(int error, const char* description)
{
	FAIL("GLFW Error: %d, %s", error, description);
}

//Handles Mouse Button presses and releases, creates mouse button event and calls OnMousePressed with new event
void GLFWCallbacks::MouseButtonCallback(GLFWwindow* w, int button, int action, int mods)
{
    if (w == nullptr) return;
    syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(w));

	SB::BasicEvents::ButtonType buttonType = SB::BasicEvents::MOUSE_BUTTON_NONE;
	SB::BasicEvents::ActionType actionType = SB::BasicEvents::ACTION_NONE;
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		buttonType = SB::BasicEvents::MOUSE_BUTTON_LEFT;
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		buttonType = SB::BasicEvents::MOUSE_BUTTON_RIGHT;
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		buttonType = SB::BasicEvents::MOUSE_BUTTON_MIDDLE;
		break;
	}
	switch (action)
	{
	case GLFW_PRESS:
		actionType = SB::BasicEvents::ACTION_PRESS;
		break;
	case GLFW_RELEASE:
		actionType = SB::BasicEvents::ACTION_RELEASE;
		break;
	}
	SB::BasicEvents::OnMouseButtonEvent buttonEvent;
	buttonEvent.button = buttonType;
	buttonEvent.action = actionType;
	app->OnMousePressed(buttonEvent);
}

//Handles mouse movement and creates a move event. Calls OnMouseMove with new event
void GLFWCallbacks::MouseMoveCallback(GLFWwindow* w, double x, double y)
{
    if (w == nullptr) return;
    syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(w));

	SB::BasicEvents::OnMouseMoveEvent moveEvent;
	moveEvent.x = static_cast<float>(x);
	moveEvent.y = static_cast<float>(y);
	glfwGetWindowSize(w, &moveEvent.window.x, &moveEvent.window.y);
	app->OnMouseMove(moveEvent);
}

//Handles mouse wheel scrolling / turning
void GLFWCallbacks::ScrollCallback(GLFWwindow* w, double xoffset, double yoffset)
{
    if (w == nullptr) return;
    syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(w));

	SB::BasicEvents::OnScrollEvent scrollEvent;
	scrollEvent.x = static_cast<float>(yoffset);
	app->OnScrollEvent(scrollEvent);
}

//Handles keyboard button presses / releases, creates a key event and calls OnKeyPressed with the new event.
void GLFWCallbacks::KeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods)
{
    if (w == nullptr) return;
    syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(w));

	SB::BasicEvents::OnKeyEvent keyEvent;
	switch (action)
	{
	case GLFW_PRESS:
		keyEvent.action = SB::BasicEvents::ACTION_PRESS;
		break;
	case GLFW_RELEASE:
		keyEvent.action = SB::BasicEvents::ACTION_RELEASE;
		break;
	case GLFW_REPEAT:
		keyEvent.action = SB::BasicEvents::ACTION_REPEAT;
		break;
	}
	switch (key)
	{
	case GLFW_KEY_BACKSPACE:
		keyEvent.key = SB::BasicEvents::KEY_BACKSPACE;
		break;
	case GLFW_KEY_G:
		keyEvent.key = SB::BasicEvents::KEY_G;
		break;
	case GLFW_KEY_W:
		keyEvent.key = SB::BasicEvents::KEY_W;
		break;
	case GLFW_KEY_A:
		keyEvent.key = SB::BasicEvents::KEY_A;
		break;
	case GLFW_KEY_S:
		keyEvent.key = SB::BasicEvents::KEY_S;
		break;
	case GLFW_KEY_D:
		keyEvent.key = SB::BasicEvents::KEY_D;
		break;
	case GLFW_KEY_B:
		keyEvent.key = SB::BasicEvents::KEY_B;
		break;
	case GLFW_KEY_C:
		keyEvent.key = SB::BasicEvents::KEY_C;
		break;
	case GLFW_KEY_E:
		keyEvent.key = SB::BasicEvents::KEY_E;
		break;
	case GLFW_KEY_F:
		keyEvent.key = SB::BasicEvents::KEY_F;
		break;
	case GLFW_KEY_H:
		keyEvent.key = SB::BasicEvents::KEY_H;
		break;
	case GLFW_KEY_J:
		keyEvent.key = SB::BasicEvents::KEY_J;
		break;
	case GLFW_KEY_K:
		keyEvent.key = SB::BasicEvents::KEY_K;
		break;
	case GLFW_KEY_L:
		keyEvent.key = SB::BasicEvents::KEY_L;
		break;
	case GLFW_KEY_M:
		keyEvent.key = SB::BasicEvents::KEY_M;
		break;
	case GLFW_KEY_N:
		keyEvent.key = SB::BasicEvents::KEY_N;
		break;
	case GLFW_KEY_O:
		keyEvent.key = SB::BasicEvents::KEY_O;
		break;
	case GLFW_KEY_P:
		keyEvent.key = SB::BasicEvents::KEY_P;
		break;
	case GLFW_KEY_R:
		keyEvent.key = SB::BasicEvents::KEY_R;
		break;
	case GLFW_KEY_T:
		keyEvent.key = SB::BasicEvents::KEY_T;
		break;
	case GLFW_KEY_Q:
		keyEvent.key = SB::BasicEvents::KEY_Q;
		break;
	case GLFW_KEY_Z:
		keyEvent.key = SB::BasicEvents::KEY_Z;
		break;
	case GLFW_KEY_X:
		keyEvent.key = SB::BasicEvents::KEY_X;
		break;
	case GLFW_KEY_V:
		keyEvent.key = SB::BasicEvents::KEY_V;
		break;
	case GLFW_KEY_Y:
		keyEvent.key = SB::BasicEvents::KEY_Y;
		break;
	case GLFW_KEY_U:
		keyEvent.key = SB::BasicEvents::KEY_U;
		break;
	case GLFW_KEY_I:
		keyEvent.key = SB::BasicEvents::KEY_I;
		break;
	case GLFW_KEY_SPACE:
		keyEvent.key = SB::BasicEvents::KEY_SPACE;
		break;
	case GLFW_KEY_F1:
		keyEvent.key = SB::BasicEvents::KEY_F1;
		break;
	case GLFW_KEY_F2:
		keyEvent.key = SB::BasicEvents::KEY_F2;
		break;
	case GLFW_KEY_F3:
		keyEvent.key = SB::BasicEvents::KEY_F3;
		break;
	case GLFW_KEY_F4:
		keyEvent.key = SB::BasicEvents::KEY_F4;
		break;
	case GLFW_KEY_F5:
		keyEvent.key = SB::BasicEvents::KEY_F5;
		break;
	case GLFW_KEY_F6:
		keyEvent.key = SB::BasicEvents::KEY_F6;
		break;
	case GLFW_KEY_F7:
		keyEvent.key = SB::BasicEvents::KEY_F7;
		break;
	case GLFW_KEY_F8:
		keyEvent.key = SB::BasicEvents::KEY_F8;
		break;
	case GLFW_KEY_F9:
		keyEvent.key = SB::BasicEvents::KEY_F9;
		break;
	case GLFW_KEY_F10:
		keyEvent.key = SB::BasicEvents::KEY_F10;
		break;
	case GLFW_KEY_F11:
		keyEvent.key = SB::BasicEvents::KEY_F11;
		break;
	case GLFW_KEY_F12:
		keyEvent.key = SB::BasicEvents::KEY_F12;
		break;
	case GLFW_KEY_LEFT:
		keyEvent.key = SB::BasicEvents::KEY_LEFT;
		break;
	case GLFW_KEY_RIGHT:
		keyEvent.key = SB::BasicEvents::KEY_RIGHT;
		break;
	case GLFW_KEY_UP:
		keyEvent.key = SB::BasicEvents::KEY_UP;
		break;
	case GLFW_KEY_DOWN:
		keyEvent.key = SB::BasicEvents::KEY_DOWN;
		break;
	case GLFW_KEY_COMMA:
		keyEvent.key = SB::BasicEvents::KEY_COMMA;
		break;
	case GLFW_KEY_PERIOD:
		keyEvent.key = SB::BasicEvents::KEY_PERIOD;
		break;
	case GLFW_KEY_LEFT_CONTROL:
		keyEvent.key = SB::BasicEvents::KEY_LEFT_CONTROL;
		break;
	case GLFW_KEY_RIGHT_CONTROL:
		keyEvent.key = SB::BasicEvents::KEY_RIGHT_CONTROL;
		break;
	case GLFW_KEY_GRAVE_ACCENT:
		keyEvent.key = SB::BasicEvents::KEY_GRAVE_ACCENT;
		break;
	case GLFW_KEY_PAGE_UP:
		keyEvent.key = SB::BasicEvents::KEY_PAGE_UP;
		break;
	case GLFW_KEY_PAGE_DOWN:
		keyEvent.key = SB::BasicEvents::KEY_PAGE_DOWN;
		break;
	case GLFW_KEY_KP_1:
		keyEvent.key = SB::BasicEvents::KEY_KP_1;
		break;
	case GLFW_KEY_KP_2:
		keyEvent.key = SB::BasicEvents::KEY_KP_2;
		break;
	case GLFW_KEY_KP_3:
		keyEvent.key = SB::BasicEvents::KEY_KP_3;
		break;
	case GLFW_KEY_KP_4:
		keyEvent.key = SB::BasicEvents::KEY_KP_4;
		break;
	case GLFW_KEY_KP_5:
		keyEvent.key = SB::BasicEvents::KEY_KP_5;
		break;
	case GLFW_KEY_KP_6:
		keyEvent.key = SB::BasicEvents::KEY_KP_6;
		break;
	case GLFW_KEY_KP_7:
		keyEvent.key = SB::BasicEvents::KEY_KP_7;
		break;
	case GLFW_KEY_KP_8:
		keyEvent.key = SB::BasicEvents::KEY_KP_8;
		break;
	case GLFW_KEY_KP_9:
		keyEvent.key = SB::BasicEvents::KEY_KP_9;
		break;
	case GLFW_KEY_UNKNOWN:
		app->OnHotKey(scancode);
		return;
		break;
	default:
		return;
	}
	keyEvent.rawKey = key;
	app->OnKeyPressed(keyEvent);
}

void GLFWCallbacks::CharCallback(GLFWwindow* w, unsigned int c)
{
    if (w == nullptr) return;
    syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(w));

	SB::BasicEvents::OnCharEvent charevent;
	charevent.c = c;
	app->OnCharEvent(charevent);
}


void GLFWCallbacks::IconificationCallback(GLFWwindow* window, int iconified)
{
    if (window == nullptr) return;
    syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(window));

	app->OnIconifyEvent(iconified == GLFW_TRUE);
}

void GLFWCallbacks::WindowCloseCallback(GLFWwindow* window)
{
#if defined _WIN32 && !defined SYGLASS_FREE
	HWND hWnd = glfwGetWin32Window(window);
	HWND parentHWND = (HWND) GetWindowLongPtr(hWnd, GWLP_USERDATA);

	RECT parentRect;
	GetWindowRect(parentHWND, &parentRect);
	SetParent(hWnd, parentHWND);
	SetWindowPos(hWnd, 0, 0, 0, parentRect.right - parentRect.left, parentRect.bottom - parentRect.top, 0);

	glfwSetWindowShouldClose(window, GL_FALSE);
#endif
}


void GLFWCallbacks::DragDropCallback(GLFWwindow* window, int count, const char ** paths)
{
#ifdef SYGLASS_FREE
	if (count != 1)
	{
		SB::NotificationMessage("Only one file can be dropped into syGlass View at a time.");
	}
	else
	{
		std::string extension = fs::path(paths[0]).extension().string();
		if (extension == ".json" || extension == ".JSON")
		{
			syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(window));
			app->CloseOpenedProject();
#ifdef DEBUG
			fs::path mouseLightProjectPath = SB::FileSystem::GetAbsolutePath(fs::path("MouseLight.syg"), SB::Location::Resources);
#else
			fs::path mouseLightProjectPath = SB::FileSystem::GetAbsolutePath(fs::path("MouseLight.syg"), SB::Location::ProgramData);
#endif
			app->OpenProjectByPath(mouseLightProjectPath.string().c_str());
			auto mouseLightJSONPath = std::string(paths[0]);

			TaskManager::GetInstance()->PushMainThreadTask(new MainThreadTask([app, mouseLightJSONPath]()
			{
				SetVar(componentColoring, true);
				app->GetCore().GetProject()->GetSWCManager()->ImportDragAndDroppedMouseLightJSON(mouseLightJSONPath);
			}));
		}
		else if ((extension == ".syg" || extension == ".SYG") && ProjectIO::IsProjectSigned(paths[0]))
		{
			syApplication* app = reinterpret_cast<syApplication*>(glfwGetWindowUserPointer(window));
			app->CloseOpenedProject();
			app->OpenProjectByPath(paths[0]);
		}
		else
		{
			SB::NotificationMessage("Only Mouselight JSON files and signed SYG files can be dropped into syGlass View.");
			return;
		}
	}
#endif
}
*/