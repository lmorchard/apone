#ifndef LUAINTERPRETER_H
#define LUAINTERPRETER_H

#include <string>
#include <functional>
#include <vector>
#include <memory>

struct lua_State;
struct luaL_Reg;

class lua_exception : public std::exception {
public:
	lua_exception(const std::string &msg) : msg(msg) {}
	virtual const char *what() const throw() { return msg.c_str(); }
private:
	std::string msg;
};


/*
template <class R> R popArg(struct lua_State *) {
}

template <> double popArg(struct lua_State *);
template <> int popArg(struct lua_State *);
template <> std::vector<std::string> popArg(struct lua_State *);
*/

// GET ARG FUNCTIONS - Gets an arg (without popping) from the stack
template <class T> T getArg(struct lua_State *L, int index) {}
template <> int getArg(struct lua_State *L, int index);
template <> uint32_t getArg(struct lua_State *L, int index);
template <> float getArg(struct lua_State *L, int index);
template <> std::string getArg(struct lua_State *L, int index);
template <> std::vector<std::string> getArg(struct lua_State *L, int index);

// PUSH ARG FUNCTIONS - Push an arg to the stack

int pushArg(struct lua_State *L, const int &r);
int pushArg(struct lua_State *L, const unsigned int &r);
int pushArg(struct lua_State *L, const double& a);
int pushArg(struct lua_State *L, const std::string& a);
int pushArg(struct lua_State *L, const std::vector<std::string>& a);

template <class F, class... A> void pushArg(struct lua_State *L, const F& first, const A& ... tail) {
	pushArg(L, first);
	pushArg(L, tail...);
}

	//void pushArg(const int& a);
	//void pushArg(const double& a);
	//void pushArg(const std::string& a);

struct FunctionCaller {
	virtual ~FunctionCaller() {};
	virtual int call() = 0;
};


template <class R, class... ARGS> struct FunctionCallerImpl : public FunctionCaller {

	virtual ~FunctionCallerImpl() {
		fprintf(stderr, "FUNCTRION CALLER DESTROY\n");
	};


	FunctionCallerImpl(struct lua_State *L, std::function<R(ARGS ... )> f) : L(L), func(f) {
	}


	template <class A> int apply() {
		return pushArg(L, func(getArg<A>(L, 1)));
	}

	template <class A, class B> int apply() {
		return pushArg(L, func(getArg<A>(L, 1), getArg<B>(L, 2)));
	}

	template <class A, class B, class C> int apply() {
		return pushArg(L, func(getArg<A>(L, 1), getArg<B>(L, 2), getArg<B>(L, 3)));
	}

	template <class A, class B, class C, class D> int apply() {
		return pushArg(L, func(getArg<A>(L, 1), getArg<B>(L, 2), getArg<B>(L, 3), getArg<B>(L, 4)));
	}

	template <class A, class B, class C, class D, class E> int apply() {
		return pushArg(L, func(getArg<A>(L, 1), getArg<B>(L, 2), getArg<B>(L, 3), getArg<B>(L, 4), getArg<B>(L, 5)));
	}

	template <class A, class B, class C, class D, class E, class F> int apply() {
		return pushArg(L, func(getArg<A>(L, 1), getArg<B>(L, 2), getArg<B>(L, 3), getArg<B>(L, 4), getArg<B>(L, 5), getArg<B>(L, 6)));
	}

	int call() override {
		return apply<ARGS...>();
	}

	struct lua_State *L;
	std::function<R(ARGS ... )> func;
};

template <class... ARGS> struct FunctionCallerImpl<void, ARGS...> : public FunctionCaller {
	FunctionCallerImpl(struct lua_State *L, std::function<void(ARGS ... )> f) : L(L), func(f) {
	}

	template <class A> void apply() {
		func(getArg<A>(L, 1));
	}

	template <class A, class B> void apply() {
		func(getArg<A>(L, 1), getArg<B>(L, 2));
	}

	template <class A, class B, class C> void apply() {
		func(getArg<A>(L, 1), getArg<B>(L, 2), getArg<C>(L, 3));
	}

	template <class A, class B, class C, class D> void apply() {
		func(getArg<A>(L, 1), getArg<B>(L, 2), getArg<C>(L, 3), getArg<D>(L, 4));
	}

	int call() override {
		apply<ARGS...>();
		return 0;
	}

	struct lua_State *L;
	std::function<void(ARGS ... )> func;
};

template <class R> struct FunctionCallerImpl<R> : public FunctionCaller {
	FunctionCallerImpl(struct lua_State *L, std::function<R()> f) : L(L), func(f) {
	}

	int call() override {
		return pushArg(L, func());
	}

	struct lua_State *L;
	std::function<R()> func;
};

template <> struct FunctionCallerImpl<void> : public FunctionCaller {
	FunctionCallerImpl(struct lua_State *L, std::function<void()> f) : L(L), func(f) {
	}

	int call() override {
		func();
		return 0;
	}

	struct lua_State *L;
	std::function<void()> func;
};

class LuaInterpreter {
public:
	LuaInterpreter();
	~LuaInterpreter();

	bool load(const std::string &code, const std::string &name = "");
	bool loadFile(const std::string &name);

	//void pushArg(const int& a);
	//void pushArg(const double& a);
	//void pushArg(const std::string& a);

	void getGlobal(const std::string &g);
	void setGlobal(const std::string &g);
	void luaCall(int nargs, int nret);
	void setOuputFunction(std::function<void(const std::string &)> f) {
		outputFunction = f;
	}

	std::vector<std::shared_ptr<FunctionCaller>> functions;


	static int proxy_func(lua_State *L);

	void createLuaClosure(const std::string &name, FunctionCaller *fc);

	//template <class R, class... A> void registerFunction(const std::string &name, std::function<R(A ... )> f) {
	//	createLuaClosure(name, new FunctionCallerImpl<R, A...>(L, f));
	//}

	template <class R> void registerFunction(const std::string &name, std::function<R()> f) {
		createLuaClosure(name, new FunctionCallerImpl<R>(L, f));
	}

	template <class R, class A> void registerFunction(const std::string &name, std::function<R(A)> f) {
		createLuaClosure(name, new FunctionCallerImpl<R, A>(L, f));
	}

	template <class R, class A, class B> void registerFunction(const std::string &name, std::function<R(A, B)> f) {
		createLuaClosure(name, new FunctionCallerImpl<R, A, B>(L, f));
	}

	template <class R, class A, class B, class C> void registerFunction(const std::string &name, std::function<R(A, B, C)> f) {
		createLuaClosure(name, new FunctionCallerImpl<R, A, B, C>(L, f));
	}

	template <class R, class A, class B, class C, class D> void registerFunction(const std::string &name, std::function<R(A, B, C, D)> f) {
		createLuaClosure(name, new FunctionCallerImpl<R, A, B, C, D>(L, f));
	}

	template <class R, class A, class B, class C, class D, class E> void registerFunction(const std::string &name, std::function<R(A, B, C, D, E)> f) {
		createLuaClosure(name, new FunctionCallerImpl<R, A, B, C, D, E>(L, f));
	}

	template <class R, class A, class B, class C, class D, class E, class F> void registerFunction(const std::string &name, std::function<R(A, B, C, D, E, F)> f) {
		createLuaClosure(name, new FunctionCallerImpl<R, A, B, C, D, E, F>(L, f));
	}


	template <class R, class... A> R call(const std::string &f, const A& ... args) {
		getGlobal(f);
		pushArg(L, args...);
		luaCall(sizeof...(args), 1);
		
		auto x = getArg<R>(L, -1);
		return x;
		//return popArg<R>(L);
	}

	template <class T> void set_global(const std::string &name, T arg) {
		pushArg(L, arg);
		setGlobal(name);
	}

	static int l_my_print(lua_State* L);
private:

	std::function<void(const std::string &)> outputFunction;
	//static const struct luaL_Reg *printlib;

	lua_State *L;
};

#endif // LUAINTERPRETER_H
