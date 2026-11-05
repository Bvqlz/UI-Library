#include "DemoGrabbableCube.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADemoGrabbableCube::ADemoGrabbableCube()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	// Engine cube is 100 cm.
	Mesh->SetRelativeScale3D(FVector(0.1f));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	Tags.Add(TEXT("Grabbable"));
}

void ADemoGrabbableCube::BeginPlay()
{
	Super::BeginPlay();
	Mesh->SetSimulatePhysics(bSimulatePhysics);
}
