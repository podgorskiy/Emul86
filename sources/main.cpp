#include "Application.h"
#include "GLFWCallbacks.h"
#include "keycode_map.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "common_gl.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <math.h>
#include <map>
#include <list>


bool        KeysDown[512];

auto start = std::chrono::steady_clock::now();

#ifdef __EMSCRIPTEN__
EM_BOOL mouseCb(int eventType, const EmscriptenMouseEvent* event, void* userData)
{
	ImGuiIO& io = ImGui::GetIO();
	if (event == NULL)
	{
		return false;
	}

	switch (eventType)
	{
	case EMSCRIPTEN_EVENT_MOUSEMOVE:
	{
		io.MousePos.x = event->canvasX;
		io.MousePos.y = event->canvasY;
		return true;
	}
	case EMSCRIPTEN_EVENT_MOUSEDOWN:
		io.MouseDown[0] = true;
		return true;
	case EMSCRIPTEN_EVENT_MOUSEUP:
		io.MouseDown[0] = false;
		return true;
	}
	return false;
}
#endif

void Update(void* window)
{
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer((GLFWwindow*)window));
	auto current_timestamp = std::chrono::steady_clock::now();

	std::chrono::duration<float> elapsed_time = (current_timestamp - start);

	ImGui_ImplGlfwGL3_NewFrame();
	
	ImGuiIO& io = ImGui::GetIO();
#ifndef __EMSCRIPTEN__
	if (glfwGetWindowAttrib((GLFWwindow*)window, GLFW_FOCUSED))
	{
		if (io.WantSetMousePos)
		{
			glfwSetCursorPos((GLFWwindow*)window, (double)io.MousePos.x, (double)io.MousePos.y); 
		}
		else
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos((GLFWwindow*)window, &mouse_x, &mouse_y);
			io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
		}
	}
	else
	{
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	}
#endif

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glfwPollEvents();

	app->Update();

	//ImGui::ShowDemoWindow();

	ImGui::Render();

#ifndef __EMSCRIPTEN__
	glfwSwapInterval(0);
#endif
	glfwSwapBuffers((GLFWwindow*)window);
}

Application* app;

int main(int argc, char **argv)
{
	printf("Compiled against GLFW %d.%d.%d\n", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

	if (!glfwInit())
	{
		printf("Failed to initialize GLFW\n");
		std::exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);


	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int widthMM, heightMM;
	glfwGetMonitorPhysicalSize(glfwGetPrimaryMonitor(), &widthMM, &heightMM);

	int dpix = static_cast<int>(round(mode->width / (widthMM / 25.4f)));
	int dpiy = static_cast<int>(round(mode->height / (heightMM / 25.4f)));

#ifndef __EMSCRIPTEN__
	int scale = static_cast<int>(round(dpix / 72.0f));
#else
	/*
	float devicePixelRatio = EM_ASM_DOUBLE_V(
	{
		if (window.devicePixelRatio != undefined)
		{
			return window.devicePixelRatio;
		}
		else
		{
			return 1.0;
		}
	}
	);
	*/
	int scale = 2;// static_cast<int>(round(devicePixelRatio));

	printf("scale %d\n", scale);
#endif
	//scale = 4;

	// text mode 80x25, 16 colors, 8 pages
	// character 8x16, so 640x400

#ifdef _DEBUG
	GLFWwindow* window = glfwCreateWindow(1200 * scale, 800 * scale, "Emul86", nullptr, nullptr);
#else
	GLFWwindow* window = glfwCreateWindow(640 * scale, 400 * scale, "Emul86", nullptr, nullptr);
#endif

#ifdef __EMSCRIPTEN__
	//emscripten_set_canvas_size(640, 400);
#endif

	if (!window)
	{
		glfwTerminate();
		printf("Failed to create window\n");
		std::exit(EXIT_FAILURE);
	}

#ifndef __EMSCRIPTEN__
	glfwMakeContextCurrent(window);
#endif

	if (gl3wInit())
	{
		printf("failed to initialize OpenGL");
		return EXIT_FAILURE;
	}

	ImGui_ImplGlfwGL3_Init(window, true);
	ImGui_ImplGlfwGL3_CreateDeviceObjects();

	ImGui::GetIO().FontGlobalScale = (float)scale;
	ImGui_ImplGlfwGL3_NewFrame();
	ImGui::Render();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	app = new Application();

	if (app->Init() != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	free(malloc(100000));

	app->SetScale(scale);
	{
		glfwSetWindowUserPointer(window, app);

		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
			app->Resize(width, height);
		});

		glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int mods)
		{
			if (action == GLFW_PRESS)
				KeysDown[key] = true;
			if (action == GLFW_RELEASE)
				KeysDown[key] = false;

			int index = (key << 4)
				| ((KeysDown[GLFW_KEY_LEFT_CONTROL] || KeysDown[GLFW_KEY_RIGHT_CONTROL]) ? CTRL : 0)
				| ((KeysDown[GLFW_KEY_LEFT_SHIFT] || KeysDown[GLFW_KEY_RIGHT_SHIFT]) ? SHIFT : 0)
				| ((KeysDown[GLFW_KEY_LEFT_ALT] || KeysDown[GLFW_KEY_RIGHT_ALT]) ? ALT : 0);
			int code = GetKeyCode(index);

			Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			if ((action == GLFW_PRESS || action == GLFW_REPEAT || action == GLFW_RELEASE) && code != -1)
			{
				app->PushKey(code, action == GLFW_RELEASE, action == GLFW_REPEAT);
			}
			
			app->GetIO().SetKeyFlags(
				  KeysDown[GLFW_KEY_RIGHT_SHIFT]
				, KeysDown[GLFW_KEY_LEFT_SHIFT]
				, KeysDown[GLFW_KEY_RIGHT_CONTROL] | KeysDown[GLFW_KEY_LEFT_CONTROL]
				, KeysDown[GLFW_KEY_RIGHT_ALT] | KeysDown[GLFW_KEY_LEFT_ALT]);

			ImGuiIO& io = ImGui::GetIO();
			if (action == GLFW_PRESS)
				io.KeysDown[key] = true;
			if (action == GLFW_RELEASE)
				io.KeysDown[key] = false;

			io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
			io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
			io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
			io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
		});

		glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int c)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.AddInputCharacter((unsigned short)c);
		});

		glfwSetScrollCallback(window, [](GLFWwindow*, double /*xoffset*/, double yoffset)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheel += (float)yoffset * 2.0f;
		});

		glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, int /*mods*/)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (button >= 0 && button < 3)
			{
				io.MouseDown[button] = action == GLFW_PRESS;
			}
		});


		auto start = std::chrono::steady_clock::now();

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);

#ifndef __EMSCRIPTEN__
		while (!glfwWindowShouldClose(window))
		{
			Update(window);
		}
#else 
		emscripten_set_mousedown_callback("#canvas", (void*)window, true, mouseCb);
		emscripten_set_mouseup_callback("#canvas", (void*)window, true, mouseCb);
		emscripten_set_mousemove_callback("#canvas", (void*)window, true, mouseCb);

		emscripten_set_main_loop_arg(Update, (void*)window, 0, 0);
#endif

#ifndef __EMSCRIPTEN__
		glfwSetWindowSizeCallback(window, nullptr);
		ImGui_ImplGlfwGL3_Shutdown();
#endif
	}


	return 0;
}
