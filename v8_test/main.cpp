#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <v8/v8.h>
#include <v8/v8-platform.h>
#include <v8/libplatform/libplatform.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>

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
	auto isolate = args.GetIsolate();
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

void sleep(const FunctionCallbackInfo<Value>& args)
{
	auto isolate = args.GetIsolate();
	HandleScope scope(isolate);

	if (args.Length() != 1)
	{
		isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter count!"));
		return;
	}

	if (!args[0]->IsNumber())
	{
		isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter types!"));
		return;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(args[0]->ToNumber()->Int32Value()));
}

struct TestObject
{
	int x = 0;
	int y = 0;

	static void get(Local<String> property, const PropertyCallbackInfo<Value>& info) 
	{
		auto isolate = info.GetIsolate();
		HandleScope scope(isolate);

		TestObject *object = (TestObject *) Local<External>::Cast(info.Holder()->GetInternalField(0))->Value();

		std::string szProperty = *String::Utf8Value(property);

		if (szProperty == "x") info.GetReturnValue().Set(object->x);
		if (szProperty == "y") info.GetReturnValue().Set(object->y);
	}

	static void set(Local<String> property, Local<Value> value, const PropertyCallbackInfo<Value>& info)
	{
		auto isolate = info.GetIsolate();
		HandleScope scope(isolate);

		TestObject *object = (TestObject *) Local<External>::Cast(info.Holder()->GetInternalField(0))->Value();

		std::string szProperty = *String::Utf8Value(property);

		if (!value->IsNumber())
			return;

		if (szProperty == "x") object->x = value->ToNumber()->Int32Value();
		if (szProperty == "y") object->y = value->ToNumber()->Int32Value();
	}

	static void sum(const FunctionCallbackInfo<Value>& args)
	{
		auto isolate = args.GetIsolate();
		HandleScope scope(isolate);

		TestObject *object = (TestObject *) Local<External>::Cast(args.This()->GetInternalField(0))->Value();

		args.GetReturnValue().Set(object->x + object->y);
	}

	static void CreateTestObject(const FunctionCallbackInfo<Value>& args)
	{
		auto isolate = args.GetIsolate();
		HandleScope scope(isolate);

		if (args.Length() != 2)
		{
			isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter count!"));
			return;
		}

		if (!args[0]->IsNumber() || !args[1]->IsNumber())
		{
			isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter types!"));
			return;
		}
		
		TestObject *pThis = new TestObject;

		pThis->x = args[0]->ToNumber()->Int32Value();
		pThis->y = args[1]->ToNumber()->Int32Value();


		static Persistent<ObjectTemplate> objectTemplateHeap;
		if (objectTemplateHeap.IsEmpty())
		{
			auto objectTemplate = ObjectTemplate::New(isolate);

			objectTemplate->SetInternalFieldCount(1);
			objectTemplate->SetNamedPropertyHandler(TestObject::get, TestObject::set);
			objectTemplate->Set(String::NewFromUtf8(isolate, "sum"), FunctionTemplate::New(isolate, TestObject::sum));

			objectTemplateHeap.Reset(isolate, objectTemplate);
		}

		auto objectTemplate = Local<ObjectTemplate>::New(isolate, objectTemplateHeap);
		
		auto object = objectTemplate->NewInstance();
		object->SetInternalField(0, External::New(isolate, pThis));

		args.GetReturnValue().Set(object);
	}

	static void DeleteTestObject(const FunctionCallbackInfo<Value>& args)
	{
		auto isolate = args.GetIsolate();
		HandleScope scope(isolate);

		if (args.Length() != 1)
		{
			isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter count!"));
			return;
		}

		if (!args[0]->IsObject())
		{
			isolate->ThrowException(String::NewFromUtf8(isolate, "Wrong parameter types!"));
			return;
		}

		auto internal = Local<External>::Cast(args[0]->ToObject()->GetInternalField(0));
		if (internal->Value() != NULL)
		{
			delete (TestObject *)internal->Value();
			args[0]->ToObject()->SetInternalField(0, External::New(isolate, 0));
		}
	}
};
Handle<Context> InitializeJS(Isolate *isolate)
{
	Handle<ObjectTemplate> global = ObjectTemplate::New(isolate);
	
	global->Set(isolate, "print", FunctionTemplate::New(isolate, print));
	global->Set(isolate, "callNTimes", FunctionTemplate::New(isolate, callNTimes));
	global->Set(isolate, "sleep", FunctionTemplate::New(isolate, sleep));
	global->Set(isolate, "CreateTestObject", FunctionTemplate::New(isolate, TestObject::CreateTestObject));
	global->Set(isolate, "DeleteTestObject", FunctionTemplate::New(isolate, TestObject::DeleteTestObject));
	global->Set(isolate, "GetTickCount", FunctionTemplate::New(isolate, [](const FunctionCallbackInfo<Value>& args) { args.GetReturnValue().Set(Number::New(args.GetIsolate(), GetTickCount())); }));

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
	V8::InitializeICU();
	V8::InitializePlatform(v8::platform::CreateDefaultPlatform());
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

		std::cout << "Finished!" << std::endl;
	}

	return ReportError(isolate, 0);
}