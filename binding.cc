#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <iostream>

#include "compression/compress.h"
#include "gemma.h"
#include "hwy/base.h"
#include "hwy/contrib/thread_pool/thread_pool.h"
#include "src/sentencepiece_processor.h"

using v8::Context;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Function;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Value;
using v8::External;

v8::Local <v8::String> InternalizedFromLatin1 (v8::Isolate * isolate, char const * str) {
  return v8::String::NewFromOneByte(isolate, reinterpret_cast<const uint8_t*>(str), v8::NewStringType::kInternalized).ToLocalChecked();
}

v8::Local<v8::String> StringFromUtf8(v8::Isolate* isolate, const char* data, int length) {
	return v8::String::NewFromUtf8(isolate, data, v8::NewStringType::kNormal, length).ToLocalChecked();
}

v8::Local <v8::FunctionTemplate> NewConstructorTemplate (v8::Isolate * isolate, v8::Local <v8::External> data, v8::FunctionCallback func, char const * name){
    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(isolate, func, data);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(InternalizedFromLatin1(isolate, name));
    return t;
}

void SetPrototypeMethod (v8::Isolate * isolate, v8::Local <v8::External> data, v8::Local <v8::FunctionTemplate> recv, char const * name, v8::FunctionCallback func){
        v8::HandleScope scope(isolate);
        recv->PrototypeTemplate()->Set(
                InternalizedFromLatin1(isolate, name),
                v8::FunctionTemplate::New(isolate, func, data, v8::Signature::New(isolate, recv))
        );
}

void ThrowTypeError(v8::Isolate* isolate, const char* message) { 
  isolate->ThrowException(v8::Exception::TypeError(StringFromUtf8(isolate, message, -1)));
}

class GemmaModel : public node::ObjectWrap {
public:

	static Local<Function> Init(Isolate* isolate, Local<External> data) {
		Local<FunctionTemplate> t = NewConstructorTemplate(isolate, data, JS_new, "GemmaModel");
		SetPrototypeMethod(isolate, data, t, "prompt", JS_prompt);
		return t->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
	}

private:
  
	explicit GemmaModel(std::string& tokenizer, std::string& model, std::string& weights) : node::ObjectWrap() {
    inner_pool = new hwy::ThreadPool(0);
    pool = new hwy::ThreadPool(4); // TODO: threads
    std::vector<std::string> arguments = {"./node", "--tokenizer", tokenizer, "--model", model, "--compressed_weights", weights};
    std::vector<char*> argv;
    for (const auto& arg : arguments)
      argv.push_back((char*)arg.data());
    argv.push_back(nullptr);

    gcpp::LoaderArgs loader(argv.size() - 1, argv.data());
    if (const char* error = loader.Validate()) {
      // TODO: throw exception v8
      HWY_ABORT("\nInvalid args: %s", error);
    }

    inference = new gcpp::InferenceArgs(argv.size() - 1, argv.data());
    gemmaModel = new gcpp::Gemma(loader, *pool);

    //inner_pool = inner_pool;
    //pool = pool;
    //inference = inference;
    //gemmaModel =  gemmaModel;
  };

	static void JS_new(const FunctionCallbackInfo<Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    
    assert(args.IsConstructCall());
    if (args.Length() < 1 || !args[0]->IsObject()) {
      return ThrowTypeError(isolate, "Expected argument to be an Object");
    }

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> obj = args[0]->ToObject(context).ToLocalChecked();

    /* Check 'tokenizer' param exists and is string */
    v8::Local<v8::String> tokenizerKey = StringFromUtf8(isolate, "tokenizer", -1);
    if (!obj->Has(context, tokenizerKey).FromJust()) {
      return ThrowTypeError(isolate, "Expected 'tokenizer'");
    }
    Local<Value> tokenizerVal = obj->Get(context, tokenizerKey).ToLocalChecked();
    if (!tokenizerVal->IsString()) {
      return ThrowTypeError(isolate, "Expected 'tokenizer' value to be string");
    }
    v8::String::Utf8Value tokenizerUtf8Value(isolate, tokenizerVal);
    std::string tokenizer(*tokenizerUtf8Value);

    /* Check 'model' param exists and is string */
    v8::Local<v8::String> modelKey = StringFromUtf8(isolate, "model", -1);
    if (!obj->Has(context, modelKey).FromJust()) {
      return ThrowTypeError(isolate, "Expected 'model'");
    }
    Local<Value> modelVal = obj->Get(context, modelKey).ToLocalChecked();
    if (!modelVal->IsString()) {
      return ThrowTypeError(isolate, "Expected 'model' value to be string");
    }
    v8::String::Utf8Value modelUtf8Value(isolate, modelVal);
    std::string model(*modelUtf8Value);

    /* Check 'compressed_weights' param exists and is string */
    v8::Local<v8::String> weightsKey = StringFromUtf8(isolate, "compressed_weights", -1);
    if (!obj->Has(context, weightsKey).FromJust()) {
      return ThrowTypeError(isolate, "Expected 'compressed_weights'");
    }
    Local<v8::Value> weightsVal = obj->Get(context, weightsKey).ToLocalChecked();
    if (!weightsVal->IsString()) {
      return ThrowTypeError(isolate, "Expected 'compressed_weights' value to be string");
    }
    v8::String::Utf8Value weightsUtf8Value(isolate, weightsVal);
    std::string weights(*weightsUtf8Value);
    
  	GemmaModel* session = new GemmaModel(tokenizer, model, weights);
		session->Wrap(args.This());
		args.GetReturnValue().Set(args.This());
	}

	static void JS_prompt(const FunctionCallbackInfo<Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();

    if (args.Length() < 1 || !args[0]->IsObject()) {
      return ThrowTypeError(isolate, "Expected argument to be an Object");
    }

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> obj = args[0]->ToObject(context).ToLocalChecked();

    /* Check 'prompt' param exists and is string */
    v8::Local<v8::String> promptsKey = StringFromUtf8(isolate, "prompt", -1);
    if (!obj->Has(context, promptsKey).FromJust()) {
      return ThrowTypeError(isolate, "Expected 'prompt'");
    }
    Local<v8::Value> promptVal = obj->Get(context, promptsKey).ToLocalChecked();
    if (!promptVal->IsString()) {
      return ThrowTypeError(isolate, "Expected 'prompt' value to be string");
    }
    v8::String::Utf8Value promptUtf8Value(isolate, promptVal);
    std::string prompt_string(*promptUtf8Value);

    /* Check 'streamToken' param exists and is function */
    v8::Local<v8::String> streamTokenKey = StringFromUtf8(isolate, "streamToken", -1);
    if (!obj->Has(context, streamTokenKey).FromJust()) {
      return ThrowTypeError(isolate, "Expected 'streamToken'");
    }
    Local<v8::Value> streamTokenVal = obj->Get(context, streamTokenKey).ToLocalChecked();
    if (!streamTokenVal->IsFunction()) {
      return ThrowTypeError(isolate, "Expected 'streamToken' value to be Function");
    }
    auto fn = streamTokenVal.As<v8::Function>();

    GemmaModel* session = Unwrap<GemmaModel>(args.This());
    std::vector<int> prompt;
    int prompt_size{};
    int abs_pos = 0;      // absolute token index over all turns
    int current_pos = 0;  // token index within the current turn
    auto stream_token = [&abs_pos,&current_pos, &prompt_size, tokenizer = &session->gemmaModel->Tokenizer(), &isolate, &obj, &fn](int token, float) {
      ++abs_pos;
      ++current_pos;
      if (current_pos >= prompt_size && token != gcpp::EOS_ID) {
        std::string token_text;
        HWY_ASSERT(tokenizer->Decode(std::vector<int>{token}, &token_text).ok());
        // std::cout << token_text << std::flush;
        
        auto token_string = StringFromUtf8(isolate, token_text.data(), -1).As<v8::Value>();
        fn->Call(isolate->GetCurrentContext(), obj, 1, &token_string);
      }
      return true;
    };
    auto accept_token = [](int) { return true; };
    std::mt19937 gen;
    gen.seed(42);
    int verbosity = 0;

    if (session->gemmaModel->model_training == gcpp::ModelTraining::GEMMA_IT) {
      // For instruction-tuned models: add control tokens.
      prompt_string = "<start_of_turn>user\n" + prompt_string +
                      "<end_of_turn>\n<start_of_turn>model\n";
      if (abs_pos > 0) {
        // Prepend "<end_of_turn>" token if this is a multi-turn dialogue
        // continuation.
        prompt_string = "<end_of_turn>\n" + prompt_string;
      }
    }

    HWY_ASSERT(session->gemmaModel->Tokenizer().Encode(prompt_string, &prompt).ok());

    // For both pre-trained and instruction-tuned models: prepend "<bos>" token
    // if needed.
    if (abs_pos == 0) {
      prompt.insert(prompt.begin(), 2);
    }

    prompt_size = prompt.size();
    
    GenerateGemma(*(session->gemmaModel), *(session->inference), prompt, abs_pos, *(session->pool), *(session->inner_pool), stream_token, accept_token, gen, verbosity);
		args.GetReturnValue().Set(args.This());
	}

  hwy::ThreadPool* inner_pool;
  hwy::ThreadPool* pool;
  gcpp::InferenceArgs* inference;
  gcpp::Gemma* gemmaModel;
};

void init(Local<Object> exports) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::External> data = v8::External::New(isolate, nullptr);
  exports->Set(context, InternalizedFromLatin1(isolate, "GemmaModel"), GemmaModel::Init(isolate, data)).FromJust();
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init)
