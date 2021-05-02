#include "common.h"

#include <iostream>
#include "managers/EntityManager.h"
#include "components.h"

#include <absl/strings/str_format.h>

#if _MSC_VER && DEBUG_TOOLS
#include <Windows.h>
class VSCoutFix
	: public std::stringbuf
{
public:
	~VSCoutFix() override { sync(); }
	int sync() override
	{
		::OutputDebugStringA(str().c_str());
		str(std::string()); // Clear the string buffer
		return 0;
	}
};
static VSCoutFix g_VSCoutFix;
#endif

#if DEBUG_TOOLS
void kaLog(std::string const& _message)
{
	Core::FrameData const& fd = Core::GetGlobalComponent<Core::FrameData>();
	std::cout << absl::StrFormat("[%d : %.2f Log]\t%s\n", fd.m_debug_frameCount, fd.m_debug_elapsedTime, _message);
#if _MSC_VER
	g_VSCoutFix.sync();
#endif
}
#endif

void InitialiseLogging()
{
#if _MSC_VER && DEBUG_TOOLS
	std::cout.rdbuf(&g_VSCoutFix);
#endif
}