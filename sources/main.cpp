#include "Application.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <imgui.h>
#include "examples/opengl3_example/imgui_impl_glfw_gl3.h"
#include <map>
#include <list>


std::map<int, int> keyCodes;
bool        KeysDown[512];
enum
{
	NORMAL = 0,
	SHIFT = 1 << 0,
	CTRL = 1 << 1,
	ALT = 1 << 2
};
std::list<int> keyBuffer;


int main(int argc, char **argv)
{
	int key;
	key = GLFW_KEY_A << 4; keyCodes[key | NORMAL] = 0x1E61; keyCodes[key | SHIFT] = 0x1E41; keyCodes[key | CTRL] = 0x1E01; keyCodes[key | ALT] = 0x1E00;
	key = GLFW_KEY_B << 4; keyCodes[key | NORMAL] = 0x3062; keyCodes[key | SHIFT] = 0x3042; keyCodes[key | CTRL] = 0x3002; keyCodes[key | ALT] = 0x3000;
	key = GLFW_KEY_C << 4; keyCodes[key | NORMAL] = 0x2E63; keyCodes[key | SHIFT] = 0x2E43; keyCodes[key | CTRL] = 0x2E03; keyCodes[key | ALT] = 0x2E00;
	key = GLFW_KEY_D << 4; keyCodes[key | NORMAL] = 0x2064; keyCodes[key | SHIFT] = 0x2044; keyCodes[key | CTRL] = 0x2004; keyCodes[key | ALT] = 0x2000;
	key = GLFW_KEY_E << 4; keyCodes[key | NORMAL] = 0x1265; keyCodes[key | SHIFT] = 0x1245; keyCodes[key | CTRL] = 0x1205; keyCodes[key | ALT] = 0x1200;
	key = GLFW_KEY_F << 4; keyCodes[key | NORMAL] = 0x2166; keyCodes[key | SHIFT] = 0x2146; keyCodes[key | CTRL] = 0x2106; keyCodes[key | ALT] = 0x2100;
	key = GLFW_KEY_G << 4; keyCodes[key | NORMAL] = 0x2267; keyCodes[key | SHIFT] = 0x2247; keyCodes[key | CTRL] = 0x2207; keyCodes[key | ALT] = 0x2200;
	key = GLFW_KEY_H << 4; keyCodes[key | NORMAL] = 0x2368; keyCodes[key | SHIFT] = 0x2348; keyCodes[key | CTRL] = 0x2308; keyCodes[key | ALT] = 0x2300;
	key = GLFW_KEY_I << 4; keyCodes[key | NORMAL] = 0x1769; keyCodes[key | SHIFT] = 0x1749; keyCodes[key | CTRL] = 0x1709; keyCodes[key | ALT] = 0x1700;
	key = GLFW_KEY_J << 4; keyCodes[key | NORMAL] = 0x246A; keyCodes[key | SHIFT] = 0x244A; keyCodes[key | CTRL] = 0x240A; keyCodes[key | ALT] = 0x2400;
	key = GLFW_KEY_K << 4; keyCodes[key | NORMAL] = 0x256B; keyCodes[key | SHIFT] = 0x254B; keyCodes[key | CTRL] = 0x250B; keyCodes[key | ALT] = 0x2500;
	key = GLFW_KEY_L << 4; keyCodes[key | NORMAL] = 0x266C; keyCodes[key | SHIFT] = 0x264C; keyCodes[key | CTRL] = 0x260C; keyCodes[key | ALT] = 0x2600;
	key = GLFW_KEY_M << 4; keyCodes[key | NORMAL] = 0x326D; keyCodes[key | SHIFT] = 0x324D; keyCodes[key | CTRL] = 0x320D; keyCodes[key | ALT] = 0x3200;
	key = GLFW_KEY_N << 4; keyCodes[key | NORMAL] = 0x316E; keyCodes[key | SHIFT] = 0x314E; keyCodes[key | CTRL] = 0x310E; keyCodes[key | ALT] = 0x3100;
	key = GLFW_KEY_O << 4; keyCodes[key | NORMAL] = 0x186F; keyCodes[key | SHIFT] = 0x184F; keyCodes[key | CTRL] = 0x180F; keyCodes[key | ALT] = 0x1800;
	key = GLFW_KEY_P << 4; keyCodes[key | NORMAL] = 0x1970; keyCodes[key | SHIFT] = 0x1950; keyCodes[key | CTRL] = 0x1910; keyCodes[key | ALT] = 0x1900;
	key = GLFW_KEY_Q << 4; keyCodes[key | NORMAL] = 0x1071; keyCodes[key | SHIFT] = 0x1051; keyCodes[key | CTRL] = 0x1011; keyCodes[key | ALT] = 0x1000;
	key = GLFW_KEY_R << 4; keyCodes[key | NORMAL] = 0x1372; keyCodes[key | SHIFT] = 0x1352; keyCodes[key | CTRL] = 0x1312; keyCodes[key | ALT] = 0x1300;
	key = GLFW_KEY_S << 4; keyCodes[key | NORMAL] = 0x1F73; keyCodes[key | SHIFT] = 0x1F53; keyCodes[key | CTRL] = 0x1F13; keyCodes[key | ALT] = 0x1F00;
	key = GLFW_KEY_T << 4; keyCodes[key | NORMAL] = 0x1474; keyCodes[key | SHIFT] = 0x1454; keyCodes[key | CTRL] = 0x1414; keyCodes[key | ALT] = 0x1400;
	key = GLFW_KEY_U << 4; keyCodes[key | NORMAL] = 0x1675; keyCodes[key | SHIFT] = 0x1655; keyCodes[key | CTRL] = 0x1615; keyCodes[key | ALT] = 0x1600;
	key = GLFW_KEY_V << 4; keyCodes[key | NORMAL] = 0x2F76; keyCodes[key | SHIFT] = 0x2F56; keyCodes[key | CTRL] = 0x2F16; keyCodes[key | ALT] = 0x2F00;
	key = GLFW_KEY_W << 4; keyCodes[key | NORMAL] = 0x1177; keyCodes[key | SHIFT] = 0x1157; keyCodes[key | CTRL] = 0x1117; keyCodes[key | ALT] = 0x1100;
	key = GLFW_KEY_X << 4; keyCodes[key | NORMAL] = 0x2D78; keyCodes[key | SHIFT] = 0x2D58; keyCodes[key | CTRL] = 0x2D18; keyCodes[key | ALT] = 0x2D00;
	key = GLFW_KEY_Y << 4; keyCodes[key | NORMAL] = 0x1579; keyCodes[key | SHIFT] = 0x1559; keyCodes[key | CTRL] = 0x1519; keyCodes[key | ALT] = 0x1500;
	key = GLFW_KEY_Z << 4; keyCodes[key | NORMAL] = 0x2C7A; keyCodes[key | SHIFT] = 0x2C5A; keyCodes[key | CTRL] = 0x2C1A; keyCodes[key | ALT] = 0x2C00;
	key = GLFW_KEY_1 << 4; keyCodes[key | NORMAL] = 0x0231; keyCodes[key | SHIFT] = 0x0221; keyCodes[key | ALT] = 0x7800;
	key = GLFW_KEY_2 << 4; keyCodes[key | NORMAL] = 0x0332; keyCodes[key | SHIFT] = 0x0340; keyCodes[key | ALT] = 0x7900; keyCodes[key | CTRL] = 0x0300;
	key = GLFW_KEY_3 << 4; keyCodes[key | NORMAL] = 0x0433; keyCodes[key | SHIFT] = 0x0423; keyCodes[key | ALT] = 0x7A00;
	key = GLFW_KEY_4 << 4; keyCodes[key | NORMAL] = 0x0534; keyCodes[key | SHIFT] = 0x0524; keyCodes[key | ALT] = 0x7B00;
	key = GLFW_KEY_5 << 4; keyCodes[key | NORMAL] = 0x0635; keyCodes[key | SHIFT] = 0x0625; keyCodes[key | ALT] = 0x7C00;
	key = GLFW_KEY_6 << 4; keyCodes[key | NORMAL] = 0x0736; keyCodes[key | SHIFT] = 0x075E; keyCodes[key | ALT] = 0x7D00; keyCodes[key | CTRL] = 0x071E;
	key = GLFW_KEY_7 << 4; keyCodes[key | NORMAL] = 0x0837; keyCodes[key | SHIFT] = 0x0826; keyCodes[key | ALT] = 0x7E00;
	key = GLFW_KEY_8 << 4; keyCodes[key | NORMAL] = 0x0938; keyCodes[key | SHIFT] = 0x092A; keyCodes[key | ALT] = 0x7F00;
	key = GLFW_KEY_9 << 4; keyCodes[key | NORMAL] = 0x0A39; keyCodes[key | SHIFT] = 0x0A28; keyCodes[key | ALT] = 0x8000;
	key = GLFW_KEY_0 << 4; keyCodes[key | NORMAL] = 0x0B30; keyCodes[key | SHIFT] = 0x0B29; keyCodes[key | ALT] = 0x8100;

	key = GLFW_KEY_F1 << 4; keyCodes[key | NORMAL] = 0x3B00; keyCodes[key | SHIFT] = 0x5400; keyCodes[key | CTRL] = 0x5E00; keyCodes[key | ALT] = 0x6800;
	key = GLFW_KEY_F2 << 4; keyCodes[key | NORMAL] = 0x3C00; keyCodes[key | SHIFT] = 0x5500; keyCodes[key | CTRL] = 0x5F00; keyCodes[key | ALT] = 0x6900;
	key = GLFW_KEY_F3 << 4; keyCodes[key | NORMAL] = 0x3D00; keyCodes[key | SHIFT] = 0x5600; keyCodes[key | CTRL] = 0x6000; keyCodes[key | ALT] = 0x6A00;
	key = GLFW_KEY_F4 << 4; keyCodes[key | NORMAL] = 0x3E00; keyCodes[key | SHIFT] = 0x5700; keyCodes[key | CTRL] = 0x6100; keyCodes[key | ALT] = 0x6B00;
	key = GLFW_KEY_F5 << 4; keyCodes[key | NORMAL] = 0x3F00; keyCodes[key | SHIFT] = 0x5800; keyCodes[key | CTRL] = 0x6200; keyCodes[key | ALT] = 0x6C00;
	key = GLFW_KEY_F6 << 4; keyCodes[key | NORMAL] = 0x4000; keyCodes[key | SHIFT] = 0x5900; keyCodes[key | CTRL] = 0x6300; keyCodes[key | ALT] = 0x6D00;
	key = GLFW_KEY_F7 << 4; keyCodes[key | NORMAL] = 0x4100; keyCodes[key | SHIFT] = 0x5A00; keyCodes[key | CTRL] = 0x6400; keyCodes[key | ALT] = 0x6E00;
	key = GLFW_KEY_F8 << 4; keyCodes[key | NORMAL] = 0x4200; keyCodes[key | SHIFT] = 0x5B00; keyCodes[key | CTRL] = 0x6500; keyCodes[key | ALT] = 0x6F00;
	key = GLFW_KEY_F9 << 4; keyCodes[key | NORMAL] = 0x4300; keyCodes[key | SHIFT] = 0x5C00; keyCodes[key | CTRL] = 0x6600; keyCodes[key | ALT] = 0x7000;
	key = GLFW_KEY_F10 << 4; keyCodes[key | NORMAL] = 0x4400; keyCodes[key | SHIFT] = 0x5D00; keyCodes[key | CTRL] = 0x6700; keyCodes[key | ALT] = 0x7100;
	key = GLFW_KEY_F11 << 4; keyCodes[key | NORMAL] = 0x8500; keyCodes[key | SHIFT] = 0x8700; keyCodes[key | CTRL] = 0x8900; keyCodes[key | ALT] = 0x8B00;
	key = GLFW_KEY_F12 << 4; keyCodes[key | NORMAL] = 0x8600; keyCodes[key | SHIFT] = 0x8800; keyCodes[key | CTRL] = 0xbA00; keyCodes[key | ALT] = 0x8C00;

	key = GLFW_KEY_1 << 4; keyCodes[key | NORMAL] = 0x0231; keyCodes[key | SHIFT] = 0x0221; keyCodes[key | CTRL] = 0x0231; keyCodes[key | ALT] = 0x7800;
	key = GLFW_KEY_2 << 4; keyCodes[key | NORMAL] = 0x0332; keyCodes[key | SHIFT] = 0x0340; keyCodes[key | CTRL] = 0x0300; keyCodes[key | ALT] = 0x7900;
	key = GLFW_KEY_3 << 4; keyCodes[key | NORMAL] = 0x0433; keyCodes[key | SHIFT] = 0x0423; keyCodes[key | CTRL] = 0x0433; keyCodes[key | ALT] = 0x7A00;
	key = GLFW_KEY_4 << 4; keyCodes[key | NORMAL] = 0x0534; keyCodes[key | SHIFT] = 0x0524; keyCodes[key | CTRL] = 0x0534; keyCodes[key | ALT] = 0x7B00;
	key = GLFW_KEY_5 << 4; keyCodes[key | NORMAL] = 0x0635; keyCodes[key | SHIFT] = 0x0625; keyCodes[key | CTRL] = 0x0635; keyCodes[key | ALT] = 0x7C00;
	key = GLFW_KEY_6 << 4; keyCodes[key | NORMAL] = 0x0736; keyCodes[key | SHIFT] = 0x075E; keyCodes[key | CTRL] = 0x07E1; keyCodes[key | ALT] = 0x7D00;
	key = GLFW_KEY_7 << 4; keyCodes[key | NORMAL] = 0x0837; keyCodes[key | SHIFT] = 0x0826; keyCodes[key | CTRL] = 0x0837; keyCodes[key | ALT] = 0x7E00;
	key = GLFW_KEY_8 << 4; keyCodes[key | NORMAL] = 0x0938; keyCodes[key | SHIFT] = 0x092A; keyCodes[key | CTRL] = 0x0938; keyCodes[key | ALT] = 0x7F00;
	key = GLFW_KEY_9 << 4; keyCodes[key | NORMAL] = 0x0A39; keyCodes[key | SHIFT] = 0x0A28; keyCodes[key | CTRL] = 0x0A39; keyCodes[key | ALT] = 0x8000;
	key = GLFW_KEY_0 << 4; keyCodes[key | NORMAL] = 0x0B30; keyCodes[key | SHIFT] = 0x0B29; keyCodes[key | CTRL] = 0x0B30; keyCodes[key | ALT] = 0x8100;

	key = GLFW_KEY_MINUS << 4; keyCodes[key | NORMAL] = 0x0C2D; keyCodes[key | SHIFT] = 0xC5F; keyCodes[key | CTRL] = 0x0C1F; keyCodes[key | ALT] = 0x8200;
	key = GLFW_KEY_EQUAL << 4; keyCodes[key | NORMAL] = 0x0D3D; keyCodes[key | SHIFT] = 0xD2B; keyCodes[key | CTRL] = 0x0D3D; keyCodes[key | ALT] = 0x8300;
	key = GLFW_KEY_LEFT_BRACKET << 4; keyCodes[key | NORMAL] = 0x1A5B; keyCodes[key | SHIFT] = 0x1A7B; keyCodes[key | CTRL] = 0x1A1B; keyCodes[key | ALT] = 0x1A00;
	key = GLFW_KEY_RIGHT_BRACKET << 4; keyCodes[key | NORMAL] = 0x1B5D; keyCodes[key | SHIFT] = 0x1B7D; keyCodes[key | CTRL] = 0x1B1D; keyCodes[key | ALT] = 0x1B00;
	key = GLFW_KEY_SEMICOLON << 4; keyCodes[key | NORMAL] = 0x273B; keyCodes[key | SHIFT] = 0x273A; keyCodes[key | CTRL] = 0x273B; keyCodes[key | ALT] = 0x2700;
	key = GLFW_KEY_APOSTROPHE << 4; keyCodes[key | NORMAL] = 0x2827; keyCodes[key | SHIFT] = 0x2822; keyCodes[key | CTRL] = 0x2827; keyCodes[key | ALT] = 0x2827;
	key = GLFW_KEY_GRAVE_ACCENT << 4; keyCodes[key | NORMAL] = 0x2960; keyCodes[key | SHIFT] = 0x297E; keyCodes[key | CTRL] = 0x2960; keyCodes[key | ALT] = 0x2960;
	key = GLFW_KEY_BACKSLASH << 4; keyCodes[key | NORMAL] = 0x2B5C; keyCodes[key | SHIFT] = 0x2B7C; keyCodes[key | CTRL] = 0x2B1C; keyCodes[key | ALT] = 0x2600;
	key = GLFW_KEY_GRAVE_ACCENT << 4; keyCodes[key | NORMAL] = 0x2960; keyCodes[key | SHIFT] = 0x297E; keyCodes[key | CTRL] = 0x2960; keyCodes[key | ALT] = 0x2960;
	key = GLFW_KEY_COMMA << 4; keyCodes[key | NORMAL] = 0x332C; keyCodes[key | SHIFT] = 0x333C; keyCodes[key | CTRL] = 0x332C; keyCodes[key | ALT] = 0x332C;
	key = GLFW_KEY_PERIOD << 4; keyCodes[key | NORMAL] = 0x342E; keyCodes[key | SHIFT] = 0x343E; keyCodes[key | CTRL] = 0x342E; keyCodes[key | ALT] = 0x342E;
	key = GLFW_KEY_SLASH << 4; keyCodes[key | NORMAL] = 0x352F; keyCodes[key | SHIFT] = 0x353F; keyCodes[key | CTRL] = 0x352F; keyCodes[key | ALT] = 0x352F;

	key = GLFW_KEY_BACKSPACE << 4; keyCodes[key | NORMAL] = 0x0E08; keyCodes[key | SHIFT] = 0x0E08; keyCodes[key | CTRL] = 0x0E7F; keyCodes[key | ALT] = 0x0E00;
	key = GLFW_KEY_DELETE << 4; keyCodes[key | NORMAL] = 0x5300; keyCodes[key | SHIFT] = 0x532E; keyCodes[key | CTRL] = 0x9300; keyCodes[key | ALT] = 0xA300;
	key = GLFW_KEY_DOWN << 4; keyCodes[key | NORMAL] = 0x5000; keyCodes[key | SHIFT] = 0x5032; keyCodes[key | CTRL] = 0x9100; keyCodes[key | ALT] = 0xA000;
	key = GLFW_KEY_END << 4; keyCodes[key | NORMAL] = 0x4F00; keyCodes[key | SHIFT] = 0x4F31; keyCodes[key | CTRL] = 0x7500; keyCodes[key | ALT] = 0x9F00;
	key = GLFW_KEY_ENTER << 4; keyCodes[key | NORMAL] = 0x1C0D; keyCodes[key | SHIFT] = 0x1C0D; keyCodes[key | CTRL] = 0x1C0A; keyCodes[key | ALT] = 0xA600;
	key = GLFW_KEY_ESCAPE << 4; keyCodes[key | NORMAL] = 0x011B; keyCodes[key | SHIFT] = 0x011B; keyCodes[key | CTRL] = 0x011B; keyCodes[key | ALT] = 0x0100;
	key = GLFW_KEY_HOME << 4; keyCodes[key | NORMAL] = 0x4700; keyCodes[key | SHIFT] = 0x4737; keyCodes[key | CTRL] = 0x7700; keyCodes[key | ALT] = 0x9700;
	key = GLFW_KEY_INSERT << 4; keyCodes[key | NORMAL] = 0x5200; keyCodes[key | SHIFT] = 0x5230; keyCodes[key | CTRL] = 0x9200; keyCodes[key | ALT] = 0xA200;
	key = GLFW_KEY_LEFT << 4; keyCodes[key | NORMAL] = 0x4B00; keyCodes[key | SHIFT] = 0x4B34; keyCodes[key | CTRL] = 0x7300; keyCodes[key | ALT] = 0x9B00;
	key = GLFW_KEY_RIGHT << 4; keyCodes[key | NORMAL] = 0x4D00; keyCodes[key | SHIFT] = 0x4D36; keyCodes[key | CTRL] = 0x7400; keyCodes[key | ALT] = 0x9D00;
	key = GLFW_KEY_SPACE << 4; keyCodes[key | NORMAL] = 0x3920; keyCodes[key | SHIFT] = 0x3920; keyCodes[key | CTRL] = 0x3920; keyCodes[key | ALT] = 0x3920;
	key = GLFW_KEY_TAB << 4; keyCodes[key | NORMAL] = 0x0F09; keyCodes[key | SHIFT] = 0x0F00; keyCodes[key | CTRL] = 0x9400; keyCodes[key | ALT] = 0xA500;
	key = GLFW_KEY_UP << 4; keyCodes[key | NORMAL] = 0x4800; keyCodes[key | SHIFT] = 0x4838; keyCodes[key | CTRL] = 0x8D00; keyCodes[key | ALT] = 0x9800;


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

	int scale = static_cast<int>(round(dpix / 72.0f));

	// text mode 80x25, 16 colors, 8 pages
	// character 8x16, so 640x400

#ifdef _DEBUG
	GLFWwindow* window = glfwCreateWindow(1200 * scale, 800 * scale, "Emul86", nullptr, nullptr);
#else
	GLFWwindow* window = glfwCreateWindow(640 * scale, 400 * scale, "Emul86", nullptr, nullptr);
#endif

	if (!window)
	{
		glfwTerminate();
		printf("Failed to create window\n");
		std::exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	
	if (gl3wInit())
	{
		printf("failed to initialize OpenGL");
		return EXIT_FAILURE;
	}

	ImGui_ImplGlfwGL3_Init(window, false);

	ImGui::GetIO().FontGlobalScale = (float)scale;

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
	style.Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	std::shared_ptr<Application> app = std::make_shared<Application>();
	if (app->Init() != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	
	app->SetScale(scale);
	{
		glfwSetWindowUserPointer(window, app.get());

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
			int code = keyCodes[index];

			Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			if (action == GLFW_RELEASE)
			{
				keyBuffer.push_back(code);
				app->GetIO().PushKey(code);
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

		glfwSetMouseButtonCallback(window, ImGui_ImplGlfwGL3_MouseButtonCallback);

		auto start = std::chrono::steady_clock::now();

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);

		while (!glfwWindowShouldClose(window))
		{
			auto current_timestamp = std::chrono::steady_clock::now();

			std::chrono::duration<float> elapsed_time = (current_timestamp - start);

			ImGui_ImplGlfwGL3_NewFrame();

			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


			if (keyBuffer.size() > 0)
			{
				if (keyBuffer.front() != 0)
				{
					if (app->KeyCallback(keyBuffer.front()))
					{
						keyBuffer.pop_front();
						if (keyBuffer.size() != 0)
						{
							app->GetIO().SetCurrentScanCode(keyBuffer.front());
						}
						else
						{
							app->GetIO().ClearCurrentCode();
						}
						app->GetIO().PopKey();
					}
				}
				else
				{
					keyBuffer.pop_front();
				}
			}
			else
			{
				app->GetIO().ClearCurrentCode();
			}

			app->Update();

			ImGui::Render();
			
			glfwSwapInterval(0);
			glfwSwapBuffers(window);

			glfwPollEvents();
		}

		glfwSetWindowSizeCallback(window, nullptr);
		ImGui_ImplGlfwGL3_Shutdown();
	}


	return 0;
}
