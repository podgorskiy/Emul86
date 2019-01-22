#pragma once

struct GLFWwindow;

namespace GLFWCallbacks
{
	void ErrorCallback(int error, const char* description);

	//Handles Mouse Button presses and releases, creates mouse button event and calls OnMousePressed with new event
	void MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/);

	//Handles mouse movement and creates a move event. Calls OnMouseMove with new event
	void MouseMoveCallback(GLFWwindow*, double x, double y);

	//Handles mouse wheel scrolling / turning
	void ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset);

	//Handles keyboard button presses / releases, creates a key event and calls OnKeyPressed with the new event.
	void KeyCallback(GLFWwindow*, int key, int, int action, int /*mods*/);

	void CharCallback(GLFWwindow*, unsigned int c);
	
	void IconificationCallback(GLFWwindow* window, int iconified);

	void WindowCloseCallback(GLFWwindow* window);

	void DragDropCallback(GLFWwindow* window, int count, const char** paths);
}
