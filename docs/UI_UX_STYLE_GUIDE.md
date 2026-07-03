# Frametee UI/UX Style Guide

This document outlines the standard guidelines and best practices for creating and modifying the user interface in **Frametee**. Since the application relies heavily on ImGui, maintaining a cohesive look and feel is essential to ensure it feels like a professional, polished piece of software rather than a disjointed set of windows.

## 1. Core Principles

- **Consistency**: Use standardized padding, rounding, and coloring across all panels and popups. Do not rely on ad-hoc styling unless absolutely necessary for a specialized context.
- **Visual Hierarchy**: Guide the user's eye using font sizing, color accents, and adequate spacing.
- **Clarity**: Avoid dense blocks of UI elements. Use padding (`igSpacing`, `igSeparator`) to separate logical groups.
- **Modernity**: Avoid the flat, sharp-edged default ImGui aesthetic in favor of slightly rounded corners, distinct borders, and a sleek dark mode.

## 2. Layout and Spacing

Spacing is the most important factor in making the UI feel breathable and clean. We use the following spacing tokens as our baseline:

| Property | Value | Description |
| :--- | :--- | :--- |
| `WindowPadding` | `(12.0, 12.0)` | Margin between window edges and content. Use `(16.0, 16.0)` for Modals/Popups. |
| `FramePadding` | `(8.0, 4.0)` | Padding inside framing elements (Buttons, Input fields). |
| `ItemSpacing` | `(8.0, 8.0)` | Horizontal and vertical spacing between individual UI widgets. |
| `ItemInnerSpacing` | `(6.0, 6.0)` | Spacing inside compound widgets (e.g., text next to a checkbox). |

### Grouping and Separation
- Use `igSpacing()` and `igSeparator()` generously between distinct logical sections of a window or popup. 
- A standard pattern is `igSpacing(); igSeparator(); igSpacing();` when separating primary content from a footer (like "Save" or "Cancel" buttons).

## 3. Borders and Rounding

Sharp corners can make the UI look rigid and outdated. Frametee employs subtle rounding on most elements.

| Property | Value | Description |
| :--- | :--- | :--- |
| `WindowRounding` | `8.0f` | Main windows and popups. |
| `ChildRounding` | `4.0f` | Inner child frames and scrolling regions. |
| `FrameRounding` | `4.0f` | Buttons, checkboxes, and text inputs. |
| `ScrollbarRounding`| `4.0f` | Scrollbar grabbers. |
| `FrameBorderSize` | `1.0f` | Adds a subtle 1px border around buttons and inputs to increase contrast. |
| `WindowBorderSize` | `1.0f` | 1px border around popup modals and main windows to detach them from the background. |

## 4. Color Palette

The global ImGui theme should be set to a dark, desaturated aesthetic. Avoid true blacks (`#000000`) and instead favor soft dark grays for backgrounds, with a clear, vibrant accent color for interaction.

*Note: In ImGui, colors are represented as 0.0 to 1.0 vectors.*

- **Background (WindowBg)**: `#1A1C1F` — Deep slate gray for the main canvas.
- **Panels/Frames (FrameBg)**: `#26282B` — Slightly lighter gray for inputs and un-hovered buttons.
- **Borders (Border)**: `#3A3F45` — Subtle border color to separate regions.
- **Accent (Button/Active)**: `#2A74DA` (Blue) — Used for primary buttons, active tabs, and sliders.
- **Text**: `#E0E4E8` — Off-white for high readability without harsh contrast. 
- **Text (Disabled)**: `#8A9199` — Muted gray for inactive elements or secondary labels.

## 5. Components

### Buttons
- **Primary Buttons**: Should use the accent color (e.g., Blue). 
- **Secondary Actions**: Keep them with the default `FrameBg` color, only highlighting on hover.
- **Lists / Selectables**: For lists (like the Recent Projects list), use `igSelectable` or transparent buttons (`(0,0,0,0)`) with a border size of `0` to keep them flat. Do not give list items strong borders unless they are actively selected.

### Tooltips
- Tooltips that display paths or descriptions must be wrapped to prevent overflowing the screen.
- Standard pattern:
  ```c
  if (igIsItemHovered(ImGuiHoveredFlags_None)) {
    if (igBeginTooltip()) {
      igPushTextWrapPos(400.0f); // Wrap at 400px
      igTextUnformatted(long_text, NULL);
      igPopTextWrapPos();
      igEndTooltip();
    }
  }
  ```

### Modals and Popups
- Always center modals on the screen.
- Ensure you lock their position using `ImGuiCond_Always` so they stay centered during window resizes:
  ```c
  ImGuiViewport *viewport = igGetMainViewport();
  ImVec2 center = { viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, 
                    viewport->WorkPos.y + viewport->WorkSize.y * 0.5f };
  igSetNextWindowPos(center, ImGuiCond_Always, (ImVec2){0.5f, 0.5f});
  ```
- Use `ImGuiWindowFlags_AlwaysAutoResize`, `ImGuiWindowFlags_NoMove`, and `ImGuiWindowFlags_NoSavedSettings` to prevent the user from breaking the modal layout.

## 6. Implementation Strategy

To prevent polluting every rendering function with `igPushStyleVar` and `igPushStyleColor` calls, you should define a global initialization function (e.g., `ui_apply_theme()`) that sets these parameters via `igGetStyle()` during the application's startup. 

Only use localized `igPushStyleVar` or `igPushStyleColor` when deviating from the standard UI guide for a highly specific widget.
