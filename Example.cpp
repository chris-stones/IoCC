
#include "IoCC.hpp"

namespace Example0 {

class IDependency {
public:
    virtual int GetData() const = 0;
};

class Dependency : public IDependency {
    int i;
public:
    Dependency(int i) : i(i) {}
    int GetData() const {
        return i;
    }
};

class IService {
public:
    virtual void DoSomthing() = 0;
};

class Service : public IService {
    std::shared_ptr<IDependency> injected_dependency0;
    std::shared_ptr<IDependency> injected_dependency1;
    int supplied_data0;
    int supplied_data1;
public:
    Service(std::shared_ptr<IDependency> injected_dependency0, std::shared_ptr<IDependency> injected_dependency1, int supplied0 = 0, int supplied1 = 0)
        :	injected_dependency0(injected_dependency0),
            injected_dependency1(injected_dependency1),
            supplied_data0(supplied0),
            supplied_data1(supplied1)
    {}

    void DoSomthing() {
        printf(" injected0 = %d\n ", injected_dependency0->GetData() );
        printf(" injected1 = %d\n ", injected_dependency1->GetData() );
        printf(" supplied0 = %d\n ", supplied_data0 );
        printf(" supplied1 = %d\n ", supplied_data1 );
    }
};

int main() {

    using ioc::IoCC;

    auto iocc = std::make_shared<IoCC>();

    // Tell IoCC how to create a Concrete implementation of IDependency with a constructor '4';
    iocc->RegisterInstantiator<std::shared_ptr<IDependency>()> (
        [] () { return std::make_shared<Dependency>( 4 ); });

    {
        // Use above instantiator.
        auto dependency = iocc->New<IDependency>();

        printf("created IDependency with data = %d\n", dependency->GetData() );
    }

    // Tell IoCC how to create a Concrete implementation of IDependency with a constructor to be passed at construction time. ;
    iocc->RegisterInstantiator<std::shared_ptr<IDependency>(int)>(
	    [] (int i) { return std::make_shared<Dependency>(i); } );

    {
        // Use above instantiator.
        int parameter0 = 20;
        auto dependency = iocc->New<IDependency>( parameter0 );
        printf("created IDependency with data = %d\n", dependency->GetData() );
    }

    // You can have more than one instantiator per contructor signature by assigning it a unique name.
    iocc->RegisterInstantiator<std::shared_ptr<IDependency>(int)> (
        "add1",
        [] (int i) { return std::make_shared<Dependency>(i+1);} );

    {
        // Use above instantiator.
        int parameter0 = 20;
        auto dependency = iocc->NewNamed<IDependency>( "add1", parameter0 );
        printf("created IDependency with data = %d\n", dependency->GetData() );
    }

    // Not everything has to be new, dependancy instances can be stored, and injected into multiple other instances.
    iocc->Store( iocc->New<IDependency>() );

    // if you want to store more than one instance of type, it will need a unique name.
    iocc->Store<IDependency>( "nine", iocc->New<IDependency>( 9 ) );

    {
        // retrieve above instances... this would normally us used inside of an instantiator.
        auto dep0 = iocc->Retrieve<IDependency>();
        auto dep1 = iocc->Retrieve<IDependency>("nine");

        printf("retrieved IDependencys with data %d and %d\n", dep0->GetData(), dep1->GetData());
    }


    // Tell IoCC how to create concrete instances of IService with injected dependencies.
    iocc->RegisterInstantiator<std::shared_ptr<IService>(void)> (
        [=] () { return std::make_shared<Service> (
                   iocc->New<IDependency>(),              // inject NEW  instance.
                   iocc->Retrieve<IDependency>("nine"));  // inject SHARED STORED named instance.
               }
        );

    {
        // use above instantiator...
        auto service = iocc->New<IService>();
        service->DoSomthing();
    }

    // Tell IoCC how to create concrete instances of IService with injected dependencies,
    //	AND extra data passed at instantation time.
    iocc->RegisterInstantiator<std::shared_ptr<IService>(int,int,int)>(
        [=] (int param0, int param1, int param2) {
            return std::make_shared<Service> (
                   iocc->New<IDependency>(param0),		  // inject NEW unnamed instance
                   iocc->Retrieve<IDependency>("nine"),   // inject shared stored named instance
                   param1,   // inject supplied parameter 0,
                   param2);  // inject supplied parameter 1
            });

    {
        // use above instantiator...
        auto service = iocc->New<IService>(123,456,789);
        service->DoSomthing();
    }

    return 0;
}

}
int main() {

    Example0::main();

    return 0;
}

