#include "SHHUD.h"
#include "SHTimeSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/WorldSettings.h"

void ASHHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	USHTimeSubsystem* TS = USHTimeSubsystem::Get(this);
	if (!TS) return;

	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	UFont* Font = GEngine->GetLargeFont();

	auto Centered = [&](const FString& Text, float Y, float Scale, const FLinearColor& Color)
	{
		float TW = 0.f, TH = 0.f;
		GetTextSize(Text, TW, TH, Font, Scale);
		DrawText(Text, Color, (W - TW) * 0.5f, Y, Font, Scale);
	};

	// Light blue tint that fades in as time slows down.
	if (!TS->bDead)
	{
		const float Dilation = GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
		const float Alpha = FMath::Clamp(1.f - Dilation, 0.f, 1.f) * 0.10f;
		DrawRect(FLinearColor(0.2f, 0.6f, 1.f, Alpha), 0.f, 0.f, W, H);
	}

	// Crosshair.
	DrawRect(FLinearColor::White, W * 0.5f - 3.f, H * 0.5f - 3.f, 6.f, 6.f);

	if (TS->bDead)
	{
		DrawRect(FLinearColor(1.f, 0.f, 0.f, 0.25f), 0.f, 0.f, W, H);
		Centered(TEXT("YOU GOT HIT"), H * 0.35f, 3.f, FLinearColor::White);
		Centered(TEXT("Press Enter to restart"), H * 0.35f + 80.f, 1.5f, FLinearColor::White);
	}
}
