//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  context.hpp
//
//   Description:  执行环境
//
//       Version:  1.0
//       Created:  2014年12月26日 17时28分52秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<cassert>

#include<map>
#include<stack>
#include<memory>
#include<string>
#include<vector>
#include<iostream>
#include<boost/variant.hpp>

namespace ezsh
{

class Variable:
    public boost::variant<
        std::string,
        std::vector<std::string>
    >
{};

class Context;
class ContextStack;
typedef std::shared_ptr<Context> ContextSPtr;

namespace details
{
    class StdStream
    {
    public:
        StdStream(std::ostream& stm)
            :strm_(std::addressof(stm))
        {}

        template<typename V>
        std::ostream& operator<<(const V& v)
        {
            return *strm_ << v;
        }

    private:
        std::ostream* strm_;
    };
}

class Context
{
    friend class ContextStack;

    Context()=default;
    Context(const ContextSPtr& f)
        :front_(f)
    {}

public:
    const ContextSPtr& frontGet() const
    {
        return front_;
    }

    details::StdStream stdCOut()
    {
        return details::StdStream(std::cout);
    }

    details::StdStream stdCErr()
    {
        return details::StdStream(std::cerr);
    }

private:
    ContextSPtr front_;
    std::map<std::string, Variable> variables_;
};

class ContextStack
{
    ContextStack()
    {
        stack_.emplace_back(ContextSPtr(new Context));
    }
public:
    const ContextSPtr& top() const
    {
        assert(!stack_.empty());
        return stack_.back();
    }

    template<typename C=Context>
    const ContextSPtr& alloc()
    {
        const ContextSPtr ptr(new C(top()));
        stack_.emplace_back(ptr);
        return top();
    }

    template<typename C=Context>
    const ContextSPtr& create()
    {
        return ContextSPtr (new C(top()));
    }

    void pop()
    {
        //最后一个Context是主Context，永远不能被弹出
        if(stack_.size()>1)
            stack_.pop_back();
    }

    static ContextStack& instance()
    {
        static ContextStack gs;
        return gs;
    }

    const ContextSPtr& mainContextGet() const
    {
        return stack_.front();
    }

private:
    std::deque<ContextSPtr> stack_;
};



}  // namespace ezsh

