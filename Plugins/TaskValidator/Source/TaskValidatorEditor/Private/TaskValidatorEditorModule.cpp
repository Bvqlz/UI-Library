#include "AssetRegistry/AssetData.h"
#include "DemoGrabbableCube.h"
#include "DemoPinchGrabber.h"
#include "Editor.h"
#include "EventTask.h"
#include "IPlacementModeModule.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "TargetZoneTask.h"
#include "TaskValidatorDirector.h"

#define LOCTEXT_NAMESPACE "TaskValidatorEditor"

/** Adds a "Task Validator" category to Place Actors so the plugin's actors are easy to find. */
class FTaskValidatorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (GEngine && GEngine->IsInitialized())
		{
			RegisterPlacementCategory();
		}
		else
		{
			FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FTaskValidatorEditorModule::RegisterPlacementCategory);
		}
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::GetOnPostEngineInit().RemoveAll(this);
		if (IPlacementModeModule::IsAvailable())
		{
			IPlacementModeModule::Get().UnregisterPlacementCategory(CategoryName);
		}
	}

private:
	void RegisterPlacementCategory()
	{
		if (!GEditor)
		{
			return;
		}

		IPlacementModeModule& Placement = IPlacementModeModule::Get();
		Placement.RegisterPlacementCategory(FPlacementCategoryInfo(
			LOCTEXT("Category", "Task Validator"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PlacementBrowser.Icons.Testing"),
			CategoryName,
			TEXT("PMTaskValidator"),
			45));

		int32 SortOrder = 0;
		auto Add = [&](UClass* Class, const FText& Label)
		{
			Placement.RegisterPlaceableItem(CategoryName, MakeShared<FPlaceableItem>(
				*Class, FAssetData(Class), NAME_None, NAME_None, TOptional<FLinearColor>(), SortOrder += 10, Label));
		};

		Add(ATaskValidatorDirector::StaticClass(), LOCTEXT("Director", "Task Validator Director"));
		Add(ATargetZoneTask::StaticClass(), LOCTEXT("TargetZone", "Target Zone Task"));
		Add(AEventTask::StaticClass(), LOCTEXT("Event", "Event Task"));
		Add(ADemoPinchGrabber::StaticClass(), LOCTEXT("Grabber", "Demo Pinch Grabber"));
		Add(ADemoGrabbableCube::StaticClass(), LOCTEXT("Cube", "Demo Grabbable Cube"));
	}

	static inline const FName CategoryName = TEXT("TaskValidator");
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTaskValidatorEditorModule, TaskValidatorEditor)
