#include <v8/v8.h>
#include <v8/v8-platform.h>
#include <v8/libplatform/libplatform.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "icui18n.lib")
#pragma comment(lib, "icuuc.lib")
#pragma comment(lib, "v8_base.lib")
#pragma comment(lib, "v8_libbase.lib")
#pragma comment(lib, "v8_libplatform.lib")
#pragma comment(lib, "v8_nosnapshot.lib")

using namespace v8;

std::string readFileContent(const std::string& path)
{
	std::stringstream ss;
	std::fstream fs(path);

	fs >> ss.rdbuf();
	return ss.str();
}

void print(const FunctionCallbackInfo<Value>& args)
{
	auto isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	for (int i = 0; i < args.Length(); i++)
	{
		if (!args[i]->IsString())
			continue;

		std::cout << *String::Utf8Value(args[i]->ToString());
	}

	std::cout << std::endl;
}

void callNTimes(const FunctionCallbackInfo<Value>& args)
{
	auto isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	if (args.Length() != 2)
	{
		isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter count!"));
		return;
	}

	if (!args[0]->IsInt32() || !args[1]->IsFunction())
	{
		isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter types!"));
		return;
	}


	Handle<Function> func = Handle<Function>::Cast(args[1]);


	for (int i = 0; i < args[0]->ToNumber()->Int32Value(); i++)
		func->Call(isolate->GetCurrentContext()->Global(), 0, {});

}

Handle<Context> InitializeJS(Isolate *isolate)
{
	Handle<ObjectTemplate> global = ObjectTemplate::New(isolate);

	global->Set(isolate, "print", FunctionTemplate::New(isolate, print));
	global->Set(isolate, "callNTimes", FunctionTemplate::New(isolate, callNTimes));

	return Context::New(isolate, NULL, global);
}

int ReportError(Isolate *isolate, TryCatch *try_catch = 0)
{
	HandleScope scope(isolate);

	struct __ {
		~__() {
			std::cin.get();
		}
	} _;

	if (try_catch == NULL)
		return 0;

	const char *what = *String::Utf8Value(try_catch->Exception());
	auto message = try_catch->Message();

	if (message.IsEmpty())
		std::cout << "Fehler: " << what << std::endl;
	else
		std::cout << "Fehler: " << what << " @ " << message->GetLineNumber() << std::endl;

	return 1;
}

int main(int argc, char* argv [])
{
	V8::Initialize();

	Isolate* isolate = Isolate::New();
	{
		Isolate::Scope isolate_scope(isolate);
		HandleScope handle_scope(isolate);

		Handle<Context> context = InitializeJS(isolate);
		Context::Scope context_scope(context);

		TryCatch try_catch;

		Local<String> code = String::NewFromUtf8(isolate, readFileContent("main.js").c_str());
		Local<Script> script = Script::Compile(code);
		if (script.IsEmpty())
			return ReportError(isolate, &try_catch);

		auto run = script->Run();
		if (run.IsEmpty())
			return ReportError(isolate, &try_catch);
	}

	return ReportError(isolate, 0);
}