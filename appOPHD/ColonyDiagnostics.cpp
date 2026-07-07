#include "ColonyDiagnostics.h"

#include "Constants/Strings.h"

#include <NAS2D/Filesystem.h>
#include <NAS2D/Utility.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <mutex>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif


namespace
{
	const std::string LogsPath{"logs/"};
	const std::string SessionLogFile{"logs/session.log"};
	const std::string ReadmeFile{"logs/README.txt"};

	std::mutex gLogMutex;
	std::string gSessionName;
	int gTurnCount{0};
	bool gInitialized{false};
	bool gSessionActive{false};

#ifdef _WIN32
	LPTOP_LEVEL_EXCEPTION_FILTER gPreviousExceptionFilter{nullptr};
#endif


	std::string currentTimestamp()
	{
		const auto now = std::chrono::system_clock::now();
		const auto time = std::chrono::system_clock::to_time_t(now);
		std::tm localTime{};
#ifdef _WIN32
		localtime_s(&localTime, &time);
#else
		localtime_r(&time, &localTime);
#endif
		std::ostringstream stream;
		stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
		return stream.str();
	}


	std::string crashTimestamp()
	{
		const auto now = std::chrono::system_clock::now();
		const auto time = std::chrono::system_clock::to_time_t(now);
		std::tm localTime{};
#ifdef _WIN32
		localtime_s(&localTime, &time);
#else
		localtime_r(&time, &localTime);
#endif
		std::ostringstream stream;
		stream << std::put_time(&localTime, "%Y%m%d_%H%M%S");
		return stream.str();
	}


	void appendToFile(const std::string& virtualPath, const std::string& line)
	{
		auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
		std::string existing;
		if (filesystem.exists(NAS2D::VirtualPath{virtualPath}))
		{
			existing = filesystem.readFile(NAS2D::VirtualPath{virtualPath});
		}
		filesystem.writeFile(NAS2D::VirtualPath{virtualPath}, existing + line);
	}


	void writeSessionLine(const std::string& line)
	{
		std::lock_guard lock{gLogMutex};
		appendToFile(SessionLogFile, "[" + currentTimestamp() + "] " + line + "\n");
	}


	void writeCrashReport(const std::string& reason, const std::string& details = {})
	{
		std::lock_guard lock{gLogMutex};

		const auto crashPath = LogsPath + "crash_" + crashTimestamp() + ".log";
		std::ostringstream report;
		report << "OutpostHD " << constants::Version << " — Ungraceful shutdown\n";
		report << "Timestamp: " << currentTimestamp() << "\n";
		report << "Reason: " << reason << "\n";
		report << "Session: " << (gSessionName.empty() ? "(none)" : gSessionName) << "\n";
		report << "Last known turn: " << gTurnCount << "\n";
		if (!details.empty())
		{
			report << "\nDetails:\n" << details << "\n";
		}
		report << "\nUpload the entire logs/ folder when reporting crashes.\n";

		auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
		filesystem.writeFile(NAS2D::VirtualPath{crashPath}, report.str());

		appendToFile(SessionLogFile, "[" + currentTimestamp() + "] CRASH: " + reason + " (see " + crashPath + ")\n");
	}


#ifdef _WIN32
	std::string describeExceptionCode(DWORD code)
	{
		switch (code)
		{
		case EXCEPTION_ACCESS_VIOLATION: return "Access violation";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array bounds exceeded";
		case EXCEPTION_BREAKPOINT: return "Breakpoint";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype misalignment";
		case EXCEPTION_FLT_DENORMAL_OPERAND: return "Float denormal operand";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float divide by zero";
		case EXCEPTION_FLT_INEXACT_RESULT: return "Float inexact result";
		case EXCEPTION_FLT_INVALID_OPERATION: return "Float invalid operation";
		case EXCEPTION_FLT_OVERFLOW: return "Float overflow";
		case EXCEPTION_FLT_STACK_CHECK: return "Float stack check";
		case EXCEPTION_FLT_UNDERFLOW: return "Float underflow";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal instruction";
		case EXCEPTION_IN_PAGE_ERROR: return "In-page error";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "Integer divide by zero";
		case EXCEPTION_INT_OVERFLOW: return "Integer overflow";
		case EXCEPTION_INVALID_DISPOSITION: return "Invalid disposition";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception";
		case EXCEPTION_PRIV_INSTRUCTION: return "Privileged instruction";
		case EXCEPTION_SINGLE_STEP: return "Single step";
		case EXCEPTION_STACK_OVERFLOW: return "Stack overflow";
		default: return "Unknown exception (0x" + std::to_string(code) + ")";
		}
	}


	std::string captureStackTrace(const CONTEXT* context)
	{
		if (!context) { return {}; }

		std::ostringstream stackTrace;
		stackTrace << "Stack trace:\n";

		const auto process = GetCurrentProcess();
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		if (!SymInitialize(process, nullptr, TRUE)) { return {}; }

		STACKFRAME64 frame{};
		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrFrame.Mode = AddrModeFlat;
		frame.AddrStack.Mode = AddrModeFlat;

#ifdef _M_X64
		frame.AddrPC.Offset = context->Rip;
		frame.AddrFrame.Offset = context->Rbp;
		frame.AddrStack.Offset = context->Rsp;
#elif defined(_M_IX86)
		frame.AddrPC.Offset = context->Eip;
		frame.AddrFrame.Offset = context->Ebp;
		frame.AddrStack.Offset = context->Esp;
#else
		return {};
#endif

		DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#ifdef _M_IX86
		machineType = IMAGE_FILE_MACHINE_I386;
#endif

		for (int frameIndex = 0; frameIndex < 24; ++frameIndex)
		{
			if (!StackWalk64(
				machineType,
				process,
				GetCurrentThread(),
				&frame,
				const_cast<CONTEXT*>(context),
				nullptr,
				SymFunctionTableAccess64,
				SymGetModuleBase64,
				nullptr))
			{
				break;
			}

			if (frame.AddrPC.Offset == 0) { break; }

			DWORD64 displacement = 0;
			alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
			auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = MAX_SYM_NAME;

			std::string symbolName = "unknown";
			if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
			{
				symbolName = symbol->Name;
			}

			IMAGEHLP_LINE64 line{};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			DWORD lineDisplacement = 0;
			if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line) && line.FileName)
			{
				stackTrace << "  #" << frameIndex << " 0x" << std::hex << frame.AddrPC.Offset << std::dec
					<< " " << symbolName << " (" << line.FileName << ":" << line.LineNumber << ")\n";
			}
			else
			{
				stackTrace << "  #" << frameIndex << " 0x" << std::hex << frame.AddrPC.Offset << std::dec
					<< " " << symbolName << "\n";
			}
		}

		return stackTrace.str();
	}


	LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
	{
		std::ostringstream details;
		if (exceptionInfo && exceptionInfo->ExceptionRecord)
		{
			const auto* record = exceptionInfo->ExceptionRecord;
			details << "Exception: " << describeExceptionCode(record->ExceptionCode) << "\n";
			details << "Address: 0x" << std::hex << reinterpret_cast<std::uintptr_t>(record->ExceptionAddress) << std::dec << "\n";

			if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
			{
				const auto operation = record->ExceptionInformation[0] == 0 ? "read" :
					record->ExceptionInformation[0] == 1 ? "write" : "execute";
				details << "Access: " << operation << " at 0x" << std::hex << record->ExceptionInformation[1] << std::dec << "\n";
			}

			const auto stackTrace = captureStackTrace(exceptionInfo->ContextRecord);
			if (!stackTrace.empty())
			{
				details << stackTrace;
			}
		}

		writeCrashReport("Unhandled Windows exception", details.str());
		if (gPreviousExceptionFilter) { return gPreviousExceptionFilter(exceptionInfo); }
		return EXCEPTION_EXECUTE_HANDLER;
	}
#endif


	void onTerminate()
	{
		std::string details;
		if (const auto current = std::current_exception())
		{
			try
			{
				std::rethrow_exception(current);
			}
			catch (const std::exception& exception)
			{
				details = exception.what();
			}
			catch (...)
			{
				details = "non-std::exception";
			}
		}

		writeCrashReport("std::terminate() called", details);
		std::abort();
	}


	void writeReadme()
	{
		const std::string readme =
			"OutpostHD diagnostic logs\n"
			"=========================\n\n"
			"session.log  — Per-turn colony snapshot and notable events during play.\n"
			"crash_*.log  — Written when the game exits ungracefully (crash, abort, etc.).\n\n"
			"If the game crashes, zip this entire logs/ folder and send it to the developer.\n";

		auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
		if (!filesystem.exists(NAS2D::VirtualPath{ReadmeFile}))
		{
			filesystem.writeFile(NAS2D::VirtualPath{ReadmeFile}, readme);
		}
	}
}


void ColonyDiagnostics::initialize()
{
	if (gInitialized) { return; }

	auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
	filesystem.makeDirectory(NAS2D::VirtualPath{LogsPath});
	writeReadme();

#ifdef _WIN32
	gPreviousExceptionFilter = SetUnhandledExceptionFilter(unhandledExceptionFilter);
#endif
	std::set_terminate(onTerminate);

	gInitialized = true;
	writeSessionLine("=== OutpostHD " + constants::Version + " started ===");
}


void ColonyDiagnostics::shutdown(bool cleanShutdown)
{
	if (!gInitialized) { return; }

	if (cleanShutdown)
	{
		writeSessionLine("=== OutpostHD shut down cleanly ===");
	}
	else
	{
		writeCrashReport("Shutdown without clean flag");
	}

	gInitialized = false;
}


void ColonyDiagnostics::beginSession(const std::string& sessionName)
{
	gSessionName = sessionName;
	gSessionActive = true;
	gTurnCount = 0;
	writeSessionLine("--- Session begin: " + sessionName + " ---");
}


void ColonyDiagnostics::endSession()
{
	if (!gSessionActive) { return; }
	writeSessionLine("--- Session end: " + gSessionName + " (turn " + std::to_string(gTurnCount) + ") ---");
	gSessionActive = false;
	gSessionName.clear();
}


void ColonyDiagnostics::setTurnCount(int turn)
{
	gTurnCount = turn;
}


void ColonyDiagnostics::logTurn(const ColonyTurnLogInfo& info)
{
	gTurnCount = info.turn;

	std::ostringstream line;
	line << "turn=" << info.turn
		<< " pop=" << info.population
		<< " structures=" << info.structures
		<< " research_completed=" << info.completedResearch
		<< " research_active=" << info.activeResearch
		<< " research_queued=" << info.queuedResearch
		<< " resources=" << info.resourcesTotal
		<< " morale=" << info.morale
		<< " food=" << info.food
		<< " robots_deployed=" << info.robotsDeployed
		<< " dozers_idle=" << info.dozersIdle;

	writeSessionLine(line.str());
}


void ColonyDiagnostics::logEvent(const std::string& message)
{
	writeSessionLine("event: " + message);
}


void ColonyDiagnostics::notifyAutoSave(int turn, const std::string& savePath)
{
	writeSessionLine("autosave turn=" + std::to_string(turn) + " path=" + savePath);
}