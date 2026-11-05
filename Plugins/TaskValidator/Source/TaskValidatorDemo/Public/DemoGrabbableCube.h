#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DemoGrabbableCube.generated.h"

class UStaticMeshComponent;

/** 10 cm cube tagged "Grabbable" for ADemoPinchGrabber. Any actor with that tag works the same way. */
UCLASS()
class TASKVALIDATORDEMO_API ADemoGrabbableCube : public AActor
{
	GENERATED_BODY()

public:
	ADemoGrabbableCube();

	UPROPERTY(VisibleAnywhere, Category = "Demo")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Off by default so the cube floats where it's released (no table needed for a demo). */
	UPROPERTY(EditAnywhere, Category = "Demo")
	bool bSimulatePhysics = false;

protected:
	virtual void BeginPlay() override;
};
