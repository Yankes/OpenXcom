/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef OPENXCOM_SCRIPTBIND_H
#define	OPENXCOM_SCRIPTBIND_H

#include "Script.h"
#include "Exception.h"
#include "Logger.h"
#include <functional>

namespace OpenXcom
{


/**
 * Hack needed by Clang 6.0 to propery transfer `auto` template parameters,
 * This need be macro becasue GCC 7.4 fail on this hack,
 * VS look it could handle both cases
 */
#ifdef __clang__
template<auto T>
static constexpr decltype(T) _clang_auto_hack()
{
    return T;
}
#define MACRO_CLANG_AUTO_HACK(T) _clang_auto_hack<T>()
#else
#define MACRO_CLANG_AUTO_HACK(T) T
#endif


/**
 * Type of code block
 */
enum BlockEnum
{
	BlockIf,
	BlockElse,
	BlockWhile,
	BlockLoop,
};


/**
 * Helper structure used by ScriptParser::parse
 */
struct ParserWriter
{
	struct Block
	{
		BlockEnum type;
		ScriptRefData nextLabel;
		ScriptRefData finalLabel;
	};

	template<typename T, typename = typename std::enable_if<std::is_pod<T>::value>::type>
	class ReservedPos
	{
		ProgPos _pos;
		ReservedPos(ProgPos pos) : _pos{ pos }
		{

		}
		ProgPos getPos()
		{
			return _pos;
		}

		friend struct ParserWriter;
	};

	class ProcOp { };

	/// member pointer accessing script operations.
	ScriptContainerBase& container;
	/// all available data for script.
	const ScriptParserBase& parser;
	/// temporary script data.
	std::vector<ScriptRefData> refListCurr;
	/// list of labels places of usage.
	std::vector<std::pair<ReservedPos<ProgPos>, int>> refLabelsUses;
	/// list of labels positions.
	std::vector<ProgPos> refLabelsList;

	/// index of used script registers.
	Uint8 regIndexUsed;
	/// negative index of used const values.
	int constIndexUsed;

	/// Store position of blocks of code like "if" or "while".
	std::vector<Block> codeBlocks;

	/// Constructor.
	ParserWriter(
			Uint8 regUsed,
			ScriptContainerBase& c,
			const ScriptParserBase& d);

	/// Finall fixes of data.
	void relese();

	/// Get referece based on name.
	ScriptRefData getReferece(const ScriptRef& s) const;
	/// Add referece based.
	ScriptRefData addReferece(const ScriptRefData& data);

	/// Get current position in proc vector.
	ProgPos getCurrPos() const;
	/// Get distance betwean two positions in proc vector.
	size_t getDiffPos(ProgPos begin, ProgPos end) const;

	/// Push zeros to fill empty space.
	ProgPos push(size_t s);
	/// Update space on proc vector.
	void update(ProgPos pos, void* data, size_t s);

	/// Preparing place and position on proc vector for some value and return position of it.
	template<typename T>
	ReservedPos<T> pushReserved()
	{
		return { ParserWriter::push(sizeof(T)) };
	}
	/// Setting previosly prepared place with value.
	template<typename T>
	void updateReserved(ReservedPos<T> pos, T value)
	{
		update(pos.getPos(), &value, sizeof(T));
	}

	/// Push custom value on proc vector.
	void pushValue(ScriptValueData v);

	/// Pushing proc operation id on proc vector.
	ReservedPos<ProcOp> pushProc(Uint8 procId);

	/// Updating previosoly added proc operation id.
	void updateProc(ReservedPos<ProcOp> pos, int procOffset);

	/// Try pushing label arg on proc vector. Can't use this to create loop back label.
	bool pushLabelTry(const ScriptRefData& data);

	/// Create new label for proc vector.
	ScriptRefData addLabel(const ScriptRef& data = {});

	/// Setting offset of label on proc vector.
	bool setLabel(const ScriptRefData& data, ProgPos offset);

	/// Try pushing reg arg on proc vector.
	template<typename T>
	bool pushConstTry(const ScriptRefData& data)
	{
		return pushConstTry(data, ScriptParserBase::getArgType<T>());
	}

	/// Try pushing data arg on proc vector.
	bool pushConstTry(const ScriptRefData& data, ArgEnum type);

	/// Try pushing reg arg on proc vector.
	template<typename T>
	bool pushRegTry(const ScriptRefData& data)
	{
		return pushRegTry(data, ScriptParserBase::getArgType<T>());
	}

	/// Try pushing reg arg on proc vector.
	bool pushRegTry(const ScriptRefData& data, ArgEnum type);

	/// Add new reg arg.
	template<typename T>
	bool addReg(const ScriptRef& s)
	{
		return addReg(s, ScriptParserBase::getArgType<T>());
	}

	/// Add new reg arg.
	bool addReg(const ScriptRef& s, ArgEnum type);
}; //struct ParserWriter


////////////////////////////////////////////////////////////
//				Mata helper classes
////////////////////////////////////////////////////////////

namespace helper
{

template<typename T, int P>
using GetType = decltype(T::typeFunc(PosTag<P>{}));


template<int MaxSize, typename... T>
struct SumListIndexImpl;

template<int MaxSize, typename T1, typename... T>
struct SumListIndexImpl<MaxSize, T1, T...> : SumListIndexImpl<MaxSize, T...>
{
	using tag = PosTag<MaxSize - (1 + sizeof...(T))>;

	using SumListIndexImpl<MaxSize, T...>::typeFunc;
	static T1 typeFunc(tag);

	static constexpr FuncCommon getDynamic(int i)
	{
		return tag::value == i ? T1::func : SumListIndexImpl<MaxSize, T...>::getDynamic(i);
	}
};

template<int MaxSize>
struct SumListIndexImpl<MaxSize>
{
	struct End
	{
		static constexpr int offset = 0;

		[[gnu::always_inline]]
		static RetEnum func(ScriptWorkerBase &, const Uint8 *, ProgPos &)
		{
			return RetError;
		}
	};
	static End typeFunc(...);
	static constexpr FuncCommon getDynamic(int i)
	{
		return End::func;
	}
};

template<typename... V>
struct SumList : SumListIndexImpl<sizeof...(V), V...>
{
	static constexpr size_t size = sizeof...(V);

	using SumListIndexImpl<sizeof...(V), V...>::typeFunc;
	using SumListIndexImpl<sizeof...(V), V...>::getDynamic;

	template<typename... F2>
	constexpr SumList<V..., F2...> operator+(SumList<F2...>) const { return {}; }
	constexpr SumList<V...> operator+() const { return {}; }
};

////////////////////////////////////////////////////////////
//						Arg class
////////////////////////////////////////////////////////////

template<typename... A>
struct Arg;

template<typename A1, typename... A2>
struct Arg<A1, A2...> : public Arg<A2...>
{
	using next = Arg<A2...>;
	using tag = PosTag<sizeof...(A2)>;

	static constexpr int offset(int i)
	{
		return i == tag::value ? A1::size : next::offset(i);
	}
	static constexpr int ver()
	{
		return tag::value + 1;
	}

	using next::typeFunc;
	static A1 typeFunc(tag);

	static int parse(ParserWriter& ph, const ScriptRefData* t)
	{
		if (A1::parse(ph, *t))
		{
			return tag::value;
		}
		else
		{
			return next::parse(ph, t);
		}
	}
	static int parse(ParserWriter& ph, const ScriptRefData** begin, const ScriptRefData* end)
	{
		if (*begin == end)
		{
			Log(LOG_ERROR) << "Not enough args in operation";
			return -1;
		}
		else
		{
			int curr = parse(ph, *begin);
			if (curr < 0)
			{
				if (**begin)
				{
					Log(LOG_ERROR) << "Incorrect type of argument '"<< (*begin)->name.toString() <<"'";
				}
				else
				{
					Log(LOG_ERROR) << "Unknown argument '"<< (*begin)->name.toString() <<"'";
				}
				return -1;
			}
			++*begin;
			return curr;
		}
	}
	template<typename A>
	static ArgEnum typeHelper()
	{
		return A::type();
	}
	static ScriptRange<ArgEnum> argTypes()
	{
		const static ArgEnum types[ver()] = { typeHelper<A1>(), typeHelper<A2>()... };
		return { std::begin(types), std::end(types) };
	}
};

template<>
struct Arg<>
{
	static constexpr int offset(int i)
	{
		return 0;
	}
	static constexpr int ver() = delete;
	static void typeFunc() = delete;

	static int parse(ParserWriter& ph, const ScriptRefData* begin)
	{
		return -1;
	}
	static ScriptRange<ArgEnum> argTypes()
	{
		return { };
	}
};

template<typename T>
struct ArgInternal
{
	using tag = PosTag<0>;

	static constexpr int offset(int i)
	{
		return 0;
	}
	static constexpr int ver()
	{
		return 1;
	};

	static T typeFunc(tag);

	static int parse(ParserWriter& ph, const ScriptRefData** begin, const ScriptRefData* end)
	{
		return 0;
	}
	static ScriptRange<ArgEnum> argTypes()
	{
		return { };
	}
};

////////////////////////////////////////////////////////////
//					ArgColection class
////////////////////////////////////////////////////////////

template<int MaxSize, typename... T>
struct ArgColection;

template<int MaxSize, typename T1, typename... T2>
struct ArgColection<MaxSize, T1, T2...> : public ArgColection<MaxSize, T2...>
{
	using next = ArgColection<MaxSize, T2...>;
	using tag = PosTag<MaxSize - (sizeof...(T2) + 1)>;

	static constexpr int pos(int ver, int pos)
	{
		return (pos == tag::value ? ver % T1::ver() : next::pos(ver / T1::ver(), pos));
	}
	static constexpr int offset(int ver, int pos)
	{
		return (pos > tag::value ? T1::offset(ver % T1::ver()) : 0) + next::offset(ver / T1::ver(), pos);
	}
	static constexpr int ver()
	{
		return T1::ver() * next::ver();
	}
	static constexpr int arg()
	{
		return MaxSize;
	}
	static int parse(ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
	{
		int curr = T1::parse(ph, &begin, end);
		if (curr < 0)
		{
			return -1;
		}
		int lower = next::parse(ph, begin, end);
		if (lower < 0)
		{
			return -1;
		}
		return lower * T1::ver() + curr;
	}
	template<typename T>
	static ScriptRange<ArgEnum> argHelper()
	{
		return T::argTypes();
	}
	static ScriptRange<ScriptRange<ArgEnum>> overloadType()
	{
		const static ScriptRange<ArgEnum> types[MaxSize] =
		{
			argHelper<T1>(),
			argHelper<T2>()...,
		};
		return { std::begin(types), std::end(types) };
	}

	using next::typeFunc;
	static T1 typeFunc(tag);
};

template<int MaxSize>
struct ArgColection<MaxSize>
{
	using tag = PosTag<-1>;

	static constexpr int pos(int ver, int pos)
	{
		return 0;
	}
	static constexpr int offset(int ver, int pos)
	{
		return 0;
	}
	static constexpr int ver()
	{
		return 1;
	}
	static constexpr int arg()
	{
		return 0;
	}
	static int parse(ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
	{
		//we shoud have used all avaiable tokens.
		if (begin == end)
		{
			return 0;
		}
		else
		{
			Log(LOG_ERROR) << "Too many args in operation";
			return -1;
		}
	}
	static ScriptRange<ScriptRange<ArgEnum>> overloadType()
	{
		return { };
	}
	static void typeFunc() = delete;
};

////////////////////////////////////////////////////////////
//						Arguments impl
////////////////////////////////////////////////////////////

struct ArgContextDef : ArgInternal<ArgContextDef>
{
	using ReturnType = ScriptWorkerBase&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw;
	}
};

struct ArgProgDef : ArgInternal<ArgProgDef>
{
	using ReturnType = ProgPos&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return curr;
	}
};

template<typename T>
struct ArgRegDef
{
	using ReturnType = T;
	static constexpr size_t size = sizeof(Uint8);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.ref<ReturnType>(*arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return ph.pushRegTry<ReturnType>(t);
	}

	static ArgEnum type()
	{
		return ArgSpecAdd(ScriptParserBase::getArgType<ReturnType>(), ArgSpecReg);
	}
};
template<typename T>
struct ArgValueDef
{
	using ReturnType = T;
	static constexpr size_t size = sizeof(T);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ReturnType>(arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return ph.pushConstTry<ReturnType>(t);
	}

	static ArgEnum type()
	{
		return ScriptParserBase::getArgType<ReturnType>();
	}
};

struct ArgLabelDef
{
	using ReturnType = ProgPos;
	static constexpr size_t size = sizeof(ReturnType);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ProgPos>(arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return ph.pushLabelTry(t);
	}

	static ArgEnum type()
	{
		return ArgLabel;
	}
};

struct ArgFuncDef
{
	using ReturnType = FuncCommon;
	static constexpr size_t size = sizeof(ReturnType);
	static FuncCommon get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<FuncCommon>(arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

template<int Size>
struct ArgRawDef
{
	using ReturnType = const Uint8*;
	static constexpr size_t size = Size;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return arg;
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

template<typename T>
struct ArgNullDef
{
	using ReturnType = T;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return ReturnType{};
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return t.type == ArgNull;
	}

	static ArgEnum type()
	{
		return ArgNull;
	}
};

////////////////////////////////////////////////////////////
//					ArgSelector class
////////////////////////////////////////////////////////////

template<>
struct ArgSelector<ScriptWorkerBase&>
{
	using type = ArgContextDef;
};

template<>
struct ArgSelector<int&>
{
	using type = Arg<ArgRegDef<int&>>;
};

template<>
struct ArgSelector<int>
{
	using type = Arg<ArgValueDef<int>, ArgRegDef<int>>;
};

template<>
struct ArgSelector<const int&> : ArgSelector<int>
{

};

template<>
struct ArgSelector<bool>
{
	using type = Arg<ArgValueDef<int>, ArgRegDef<int>>;
};

template<typename T, typename I>
struct ArgSelector<ScriptTag<T, I>>
{
	using type = Arg<ArgValueDef<ScriptTag<T, I>>, ArgRegDef<ScriptTag<T, I>>, ArgNullDef<ScriptTag<T, I>>>;
};

template<typename T, typename I>
struct ArgSelector<ScriptTag<T, I>&>
{
	using type = Arg<ArgRegDef<ScriptTag<T, I>&>>;
};

template<typename T, typename I>
struct ArgSelector<const ScriptTag<T, I>&> : ArgSelector<ScriptTag<T, I>>
{

};

template<>
struct ArgSelector<ProgPos&>
{
	using type = ArgProgDef;
};

template<>
struct ArgSelector<ProgPos>
{
	using type = Arg<ArgLabelDef>;
};

template<>
struct ArgSelector<FuncCommon>
{
	using type = Arg<ArgFuncDef>;
};

template<>
struct ArgSelector<const Uint8*>
{
	using type = Arg<ArgRawDef<64>, ArgRawDef<32>, ArgRawDef<16>, ArgRawDef<8>, ArgRawDef<4>, ArgRawDef<0>>;
};

template<typename T>
struct ArgSelector<T*>
{
	using type = Arg<ArgRegDef<T*>, ArgValueDef<T*>, ArgNullDef<T*>>;
};

template<typename T>
struct ArgSelector<T*&>
{
	using type = Arg<ArgRegDef<T*&>>;
};

template<typename T>
struct ArgSelector<const T*>
{
	using type = Arg<ArgRegDef<const T*>, ArgValueDef<const T*>, ArgNullDef<const T*>>;
};

template<typename T>
struct ArgSelector<const T*&>
{
	using type = Arg<ArgRegDef<const T*&>>;
};

template<>
struct ArgSelector<std::nullptr_t>
{
	using type = Arg<ArgNullDef<std::nullptr_t>>;
};

template<typename T>
struct GetArgsImpl;

template<typename... Args>
struct GetArgsImpl<RetEnum(Args...)>
{
	using type = ArgColection<sizeof...(Args), typename ArgSelector<Args>::type...>;
};

template<typename Func>
using GetArgs = typename GetArgsImpl<decltype(Func::func)>::type;

////////////////////////////////////////////////////////////
//				FuncVer and FuncGroup class
////////////////////////////////////////////////////////////

template<typename Func, int Ver, typename PosList = MakeListTag<GetArgs<Func>::arg()>>
struct FuncVer;

template<typename Func, int Ver, int... Pos>
struct FuncVer<Func, Ver, ListTag<Pos...>>
{
	using Args = GetArgs<Func>;
	static constexpr int offset = Args::offset(Ver, Args::arg());

	template<int CurrPos>
	using GetTypeAt = GetType<GetType<Args, CurrPos>, Args::pos(Ver, CurrPos)>;

	template<int CurrPos>
	[[gnu::always_inline]]
	static typename GetTypeAt<CurrPos>::ReturnType get(ScriptWorkerBase& sw, const Uint8* procArgs, ProgPos& curr)
	{
		constexpr int offs = Args::offset(Ver, CurrPos);
		return GetTypeAt<CurrPos>::get(sw, procArgs + offs, curr);
	}

	[[gnu::always_inline]]
	static RetEnum func(ScriptWorkerBase& sw, const Uint8* procArgs, ProgPos& curr)
	{
		return Func::func(get<Pos>(sw, procArgs, curr)...);
	}
};

template<typename Func, typename VerList = MakeListTag<GetArgs<Func>::ver()>>
struct FuncGroup;

template<typename Func, int... Ver>
struct FuncGroup<Func, ListTag<Ver...>> : GetArgs<Func>
{
	using FuncList = SumList<FuncVer<Func, Ver>...>;
	using GetArgs<Func>::ver;
	using GetArgs<Func>::arg;
	using GetArgs<Func>::parse;
	using GetArgs<Func>::overloadType;

	static constexpr FuncCommon getDynamic(int i) { return FuncList::getDynamic(i); }
};

////////////////////////////////////////////////////////////
//					Bind helper classes
////////////////////////////////////////////////////////////

template<auto Ptr, auto... Rest>
struct BindMemberInvoke
{
    template<typename T, typename... TRest>
    static auto f(T&& a, TRest&&... b) -> decltype(auto)
    {
        using ReturnType = std::invoke_result_t<decltype(Ptr), T, TRest...>;

        ReturnType v = std::invoke(Ptr, std::forward<T>(a), std::forward<TRest>(b)...);
        if constexpr (sizeof...(Rest) > 0)
        {
            return BindMemberInvoke<Rest...>::f(std::forward<ReturnType>(v));
        }
        else
        {
            return std::forward<ReturnType>(v);
        }
    }
};

template<typename T, auto... Rest>
using BindMemberFinalType = std::decay_t<decltype(BindMemberInvoke<Rest...>::f(std::declval<T>()))>;

template<typename T>
struct BindSet
{
	static RetEnum func(T& t, T r)
	{
		t = r;
		return RetContinue;
	}
};
template<typename T>
struct BindSwap
{
	static RetEnum func(T& t, T& r)
	{
		std::swap(t, r);
		return RetContinue;
	}
};
template<typename T>
struct BindEq
{
	static RetEnum func(ProgPos& Prog, T t1, T t2, ProgPos LabelTrue, ProgPos LabelFalse)
	{
		Prog = (t1 == t2) ? LabelTrue : LabelFalse;
		return RetContinue;
	}
};
template<typename T>
struct BindClear
{
	static RetEnum func(T& t)
	{
		t = T{};
		return RetContinue;
	}
};

template<typename T, auto... X>
struct BindPropGet
{
	static RetEnum func(const T* t, BindMemberFinalType<T, X...>& p)
	{
		if (t) p = BindMemberInvoke<X...>::f(t); else p = BindMemberFinalType<T, X...>{};
		return RetContinue;
	}
};
template<typename T, auto... X>
struct BindPropSet
{
	static RetEnum func(T* t, BindMemberFinalType<T, X...> p)
	{
		if (t) BindMemberInvoke<X...>::f(t) = p;
		return RetContinue;
	}
};

template<typename T, ScriptValues<T> T::*X>
struct BindScriptValueGet
{
	static RetEnum func(const T* t, int& p, typename ScriptValues<T>::Tag st)
	{
		if (t) p = (t->*X).get(st); else p = int{};
		return RetContinue;
	}
};
template<typename T, ScriptValues<T> T::*X>
struct BindScriptValueSet
{
	static RetEnum func(T* t, typename ScriptValues<T>::Tag st, int p)
	{
		if (t) (t->*X).set(st, p);
		return RetContinue;
	}
};

template<typename T, typename N, typename P, N T::*X, P N::*XX>
struct BindPropNestGet
{
	static RetEnum func(const T* t, P& p)
	{
		if (t) p = (t->*X).*XX; else p = P{};
		return RetContinue;
	}
};
template<typename T, typename N, typename P, N T::*X, P N::*XX>
struct BindPropNestSet
{
	static RetEnum func(T* t, P p)
	{
		if (t) (t->*X).*XX = p;
		return RetContinue;
	}
};

template<typename T, typename P, P I>
struct BindValue
{
	static RetEnum func(T* t, P& p)
	{
		if (t) p = I; else p = P{};
		return RetContinue;
	}
};

template<typename T, std::string (*X)(const T*)>
struct BindDebugDisplay
{
	static RetEnum func(ScriptWorkerBase& swb, const T* t)
	{
		swb.log_buffer_add(X(t));
		return RetContinue;
	}
};

template<auto f>
struct BindFunc;

template<typename... Args, void(*X)(Args...)>
struct BindFunc<X>
{
	static RetEnum func(Args... a)
	{
		X(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...)>
struct BindFunc<X>
{
	static RetEnum func(T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...) const>
struct BindFunc<X>
{
	static RetEnum func(const T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename R, typename... Args, R(T::*X)(Args...)>
struct BindFunc<X>
{
	static RetEnum func(T* t, R& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename R, typename... Args, R(T::*X)(Args...) const>
struct BindFunc<X>
{
	static RetEnum func(const T* t, R& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename P, typename... Args, P*(T::*X)(Args...)>
struct BindFunc<X>
{
	static RetEnum func(T* t, const P*& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename P, typename... Args, P*(T::*X)(Args...) const>
struct BindFunc<X>
{
	static RetEnum func(const T* t, const P*& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename... Args, void(T::*X)(Args...)>
struct BindFunc<X>
{
	static RetEnum func(T* t, Args... a)
	{
		if (t) (t->*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

} //namespace helper

////////////////////////////////////////////////////////////
//						Bind class
////////////////////////////////////////////////////////////

struct BindBase
{
	constexpr static const char* functionWithoutDescription = "-";
	constexpr static const char* functionInvisible = "";

	ScriptParserBase* parser;
	BindBase(ScriptParserBase* p) : parser{ p }
	{

	}

	template<typename X>
	void addCustomFunc(const std::string& name, const std::string& description = functionWithoutDescription)
	{
		parser->addParser<helper::FuncGroup<X>>(name, description);
	}
	void addCustomConst(const std::string& name, int i)
	{
		parser->addConst(name, i);
	}
	template<typename T>
	void addCustomPtr(const std::string& name, T* p)
	{
		parser->addConst(name, p);
	}
};

template<typename T>
struct Bind : BindBase
{

	Bind(ScriptParserBase* p) : BindBase{ p }
	{
		parser->addParser<helper::FuncGroup<helper::BindSet<T*>>>("set", "arg1 = arg2");
		parser->addParser<helper::FuncGroup<helper::BindSet<const T*>>>("set", "arg1 = arg2");
		parser->addParser<helper::FuncGroup<helper::BindSwap<T*>>>("swap", "Swap value of arg1 and arg2");
		parser->addParser<helper::FuncGroup<helper::BindSwap<const T*>>>("swap", "Swap value of arg1 and arg2");
		parser->addParser<helper::FuncGroup<helper::BindClear<T*>>>("clear", "arg1 = null");
		parser->addParser<helper::FuncGroup<helper::BindClear<const T*>>>("clear", "arg1 = null");
		parser->addParser<helper::FuncGroup<helper::BindEq<const T*>>>("test_eq", BindBase::functionInvisible);
	}

	std::string getName(const std::string& s)
	{
		return std::string{ T::ScriptName } + "." + s;
	}

	template<typename X>
	void addFunc(const std::string& name)
	{
		addCustomFunc<X>(getName(name));
	}
	template<typename X>
	void addFunc(const std::string& name, const std::string& description)
	{
		addCustomFunc<X>(getName(name), description);
	}

	template<int T::*X>
	void addField(const std::string& get, const std::string& set = "")
	{
		addCustomFunc<helper::BindPropGet<T, X>>(getName(get), "Get int field of " + std::string{ T::ScriptName });
		if (!set.empty())
		{
			addCustomFunc<helper::BindPropSet<T, X>>(getName(set), "Set int field of " + std::string{ T::ScriptName });
		}
	}
	template<auto MemPtr0, auto MemPtr1, auto... MemPtrR>
	void addField(const std::string& get)
	{
		addCustomFunc<helper::BindPropGet<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtr1), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(get), "Get int field of " + std::string{ T::ScriptName });
	}

	void addScriptTag()
	{
		using Tag = typename ScriptValues<T>::Tag;
		parser->getGlobal()->addTagType<Tag>();
		if (!parser->haveType<Tag>())
		{
			parser->addType<Tag>(getName("Tag"));
			parser->addParser<helper::FuncGroup<helper::BindSet<Tag>>>("set", "arg1 = arg2");
			parser->addParser<helper::FuncGroup<helper::BindSwap<Tag>>>("swap", "Swap value of arg1 and arg2");
			parser->addParser<helper::FuncGroup<helper::BindClear<Tag>>>("clear", "arg1 = null");
			parser->addParser<helper::FuncGroup<helper::BindEq<Tag>>>("test_eq", BindBase::functionInvisible);
		}
	}
	template<ScriptValues<T> T::*X>
	void addScriptValue(bool canEdit = true)
	{
		addScriptTag();
		addCustomFunc<helper::BindScriptValueGet<T, X>>(getName("getTag"), "Get tag of " + std::string{ T::ScriptName });
		if (canEdit)
		{
			addCustomFunc<helper::BindScriptValueSet<T, X>>(getName("setTag"), "Set tag of " + std::string{ T::ScriptName });
		}
	}
	template<std::string (*X)(const T*)>
	void addDebugDisplay()
	{
		addCustomFunc<helper::BindDebugDisplay<T, X>>("debug_impl", BindBase::functionInvisible);
	}
	template<int X>
	void addFake(const std::string& get)
	{
		addCustomFunc<helper::BindValue<T, int, X>>(getName(get), "Get int field of " + std::string{ T::ScriptName });
	}

	template<typename P, P* (T::*X)(), const P* (T::*Y)() const>
	void addPair(const std::string& get)
	{
		add<X>(get);
		add<Y>(get);
	}

	template<typename P, const P* (T::*Y)() const>
	void addRules(const std::string& get)
	{
		add<Y>(get);
	}

	template<auto X>
	void add(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(X)>>(getName(func), description);
	}
};

} //namespace OpenXcom

#endif	/* OPENXCOM_SCRIPTBIND_H */

