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

	if (TS->bWon)
	{
		DrawRect(FLinearColor(0.2f, 1.f, 0.5f, 0.25f), 0.f, 0.f, W, H);
		Centered(TEXT("YOU ESCAPED"), H * 0.30f, 3.f, FLinearColor::White);
		Centered(FString::Printf(TEXT("Time: %.1f seconds"), TS->WinTime), H * 0.30f + 80.f, 2.f, FLinearColor::White);
		Centered(TEXT("Press Enter to play again"), H * 0.30f + 140.f, 1.5f, FLinearColor::White);
		return;
	}

	// Run timer (top right).
	{
		const FString T = FString::Printf(TEXT("%.1f"), TS->RunTime);
		float TW = 0.f, TH = 0.f;
		GetTextSize(T, TW, TH, Font, 1.5f);
		DrawText(T, FLinearColor::White, W - TW - 30.f, 20.f, Font, 1.5f);
	}

	// Light blue tint that fades in as time slows down.
	if (!TS->bDead && !TS->bRewinding)
	{
		const float Dilation = GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
		const float Alpha = FMath::Clamp(1.f - Dilation, 0.f, 1.f) * 0.10f;
		DrawRect(FLinearColor(0.2f, 0.6f, 1.f, Alpha), 0.f, 0.f, W, H);
	}

	// Crosshair.
	DrawRect(FLinearColor::White, W * 0.5f - 3.f, H * 0.5f - 3.f, 6.f, 6.f);

	// Rewind meter.
	const float BarW = 300.f, BarH = 12.f;
	const float X = (W - BarW) * 0.5f, Y = H - 60.f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X - 2.f, Y - 2.f, BarW + 4.f, BarH + 4.f);
	DrawRect(FLinearColor(1.f, 0.42f, 0.1f, 1.f), X, Y, BarW * (TS->RewindMeter / USHTimeSubsystem::MaxRewind), BarH);

	if (TS->bRewinding)
	{
		DrawRect(FLinearColor(0.5f, 0.2f, 1.f, 0.15f), 0.f, 0.f, W, H);
		Centered(TEXT("<< REWINDING"), H * 0.15f, 2.f, FLinearColor::White);
	}

	if (TS->bDead)
	{
		DrawRect(FLinearColor(1.f, 0.f, 0.f, 0.25f), 0.f, 0.f, W, H);
		Centered(TEXT("YOU GOT HIT"), H * 0.35f, 3.f, FLinearColor::White);
		Centered(TEXT("Hold R to rewind   |   Enter to restart"), H * 0.35f + 80.f, 1.5f, FLinearColor::White);
	}
}
