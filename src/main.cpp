#include "battery/allocator.hpp"
#include "lala/XCSP3_parser.hpp"
#include "lala/logic/env.hpp"
#include "lala/vstore.hpp"
#include "lala/pc.hpp"
#include "lala/octagon.hpp"
#include "lala/interval.hpp"
#include "lala/interpretation.hpp"
#include "lala/fixpoint.hpp"
#include "argparse/argparse.hpp"

#include "lala/flatzinc_parser.hpp"


using namespace lala;
using namespace battery;

using F = TFormula<standard_allocator>;

using zi = local::ZInc;
using zd = local::ZDec;
using Itv = Interval<zi>;
using IStore = VStore<Itv, standard_allocator>;
using IPC = PC<IStore>; 
using Oct = Octagon<Itv, standard_allocator>;


template<typename T>
bool solve(const F& f, VarEnv<standard_allocator>& env, IDiagnostics& diagnostics,FlatZincOutput<battery::standard_allocator> & output) {
    auto opt = create_and_interpret_and_tell<T,true>(f, env, diagnostics);
    if(!opt.has_value() || diagnostics.is_fatal()){
        std::cerr<<"Interpretation failed"<<std::endl;
        diagnostics.print();
        return false;
    }

    T abs = std::move(opt.value());

    GaussSeidelIteration{}.fixpoint(abs);
    abs.deinterpret(env).print(false);
    std::cout<<std::endl;
    auto vars = output.getOutputVars();
    for(int i=0;i<vars.size();i++){
        auto name = vars[i];
        auto var = env.variable_of(name).value();
        auto itv = abs.project(var.avars[0]).value();
        std::cout<<"s " <<name.data()<<": "<<battery::get<1>(itv)<<" "<< battery::get<0>(itv)<<std::endl;
    }

    std::cout<<"v <instantiation id='sol1' type='solution'> <list> s[] </list> <values>";
    
    for(int i=0;i<vars.size();i++){
        auto name = vars[i];
        auto var = env.variable_of(name).value();
        auto itv = abs.project(var.avars[0]).value();
        std::cout<<" "<<battery::get<1>(itv);
    }

    std::cout<<"</values> </instantiation>"<<std::endl;


    return true;
}


int main(int argc, char const *argv[])
{
    argparse::ArgumentParser program("lalaoctagonpc", "1.0", argparse::default_arguments::all);


    program.add_argument("input")
    .help("Input file name");


    program.add_argument("kind")
    .help("Type of propagation");
    

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }
    battery::standard_allocator allocator;
    FlatZincOutput<battery::standard_allocator> output(allocator);
    auto f = lala::parse_xcsp3<battery::standard_allocator>(program.get<std::string>("input"), output, TableDecomposition::ELEMENTS);
    
    VarEnv<standard_allocator> env;
    IDiagnostics diagnostics;


    if(program.get<std::string>("kind") == "pc"){
        auto result = solve<IPC>(*f, env, diagnostics,output);
        std::cout<<"result: " <<result<<std::endl;

    }
    else if(program.get<std::string>("kind") == "octagon"){
        auto result = solve<Oct>(*f, env, diagnostics,output);
        std::cout<<"result: " <<result<<std::endl;

    }else{
        std::cerr<< program.get<std::string>("kind") << " is not a valid kind of propagation"<<std::endl;
        return 1;
    }

    return 0;
}
