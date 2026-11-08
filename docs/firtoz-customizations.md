# firtoz's Frametop customizations

This is the change log for the personal integration branch, `firtoz/frametop:main`.
It lists the extra behavior maintained here, not Frametop's complete upstream feature set.
Follow each PR link for its current review/merge status; a submitted PR is not necessarily in upstream yet.

| Customization | Upstream contribution |
| --- | --- |
| Keyboard focus follows controller selection of native windows | [Frametop/frametop #87](https://github.com/Frametop/frametop/pull/87) |
| Configurable drag-zone padding, preview and styling | [Frametop/frametop #92](https://github.com/Frametop/frametop/pull/92) |
| Crisp, configurable resize-corner feedback | Local development; no upstream PR yet |
| Per-app native dashboard launch exceptions (Konsole) | Local development; no upstream PR yet |

## Controller-selected native windows get keyboard focus

After typing in a Frametop window, selecting a native SteamOS window with a controller laser now hands physical-keyboard input back to that window. Previously, a separate 3D-mouse click was needed. The ProtoArc keyboard/touchpad should use **Keyboard + 3D pointer** (`combo`) mode to avoid duplicate typing.

The fix reuses the dashboard focus-stack reader, without intercepting controller buttons. It was manually tested by switching typing between a Frametop text field and native Konsole with the controller alone. See [PR #87](https://github.com/Frametop/frametop/pull/87) for implementation and validation details.

## Configurable drag zones

**Frametop Display Settings → Visibility & pins → Window controls** provides:

- Independent horizontal and vertical padding in **centimetres per side**, from 0 to 50 cm; default 2 cm. For example, 2 cm horizontal padding makes the target 4 cm wider than the visible pill.
- No multiplier mode. The visible pill continues to auto-size, but the added physical padding stays fixed.
- **Show drag zone**, with a translucent fill and thin outline.
- **Keep drag zones visible**, while their panels are visible; it does not force other buttons to stay visible or bypass hiding/pausing.
- Independent fill/outline colors and opacity, plus outline thickness.
- Live changes, persisted in the configuration file. Targets remain clear of content and neighboring buttons.

See [Floating windows](floating-windows.md) for configuration keys and details.

Validation: manually exercised on the headset; the clean contribution passed a full ARM64 image build, all `just test-c` checks and changed-file Ruff. Relevant Python tests passed (42 in the checkout; 40 in the image, where the two documentation checks skip without Git metadata). See [PR #92](https://github.com/Frametop/frametop/pull/92) for details and broader-suite limitations.

### Screenshots

Settings UI captured directly, without VR projection, using the current preferences:

![Drag-zone padding and preview controls](images/drag-zone-settings.png)

Actual VR drag-zone preview, cropped from the headset view:

![Orange drag zone and outlined hit target surrounding the visible bar](images/drag-zone-vr.png)

Only these two wearer-reviewed crops are published. The uncropped capture stays in ignored local artifacts; screenshot review is required before publishing new captures, which may include private windows or room imagery.

## Crisp resize corners

Controller-collision follow-up: the mask had been sent in bottom-up UV coordinates instead of top-left image coordinates, vertically mirroring controller collision relative to the preview. Convert circles and rectangles correctly at the OpenVR boundary, and keep the invisible texture, mouse scale and mask at the same 256×256 size. Screen/pointer builds and geometry/protocol regression tests pass, including converted-mask equivalence across all four corners. Installed from source at `f355065` with the wearer-approved desktop restart; the running binary matches the build. SteamVR and the pointer helper were not restarted. Wearer validation remains pending.

Shape-target correction: replaces the offset square with padding around the actual corner path, including both arms and the rounded bend, using a stable envelope around all hover-animation states. **Hover padding around outline** defaults to 1 cm; **Screen exclusion margin** defaults to 0.5 cm. The outline moves farther outward automatically when needed to protect that margin. Controller masks, 3D-pointer filtering and the preview share the same bounded primitive union. Near-hand grab reach is unchanged. Both components build; geometry/protocol tests, all 41 relevant Python tests and changed-file Ruff pass. The full C/C++ recipe passes its available checks (ncnn-linked hand tests and one gaze test skip because their build dependencies are absent in this test container). Installed from source at `2823d1f` with wearer-approved desktop and pointer-service restarts; both services are active and both running binaries match the builds. SteamVR was not restarted. Wearer review remains pending; no new captures have been published.

Latest follow-up: 3 mm default outline thickness (saved choices preserved), reversible 180 ms eased hover animation, a diagnostic hover-target preview checkbox, and independent always-show checkboxes for all four corners. Multiple persistent corners are visual-only; the existing interactive target still follows the approached corner. Visibility never bypasses hidden/minimized/paused panels. Tests cover all 16 persistent-corner combinations, settings persistence/live IPC, animation endpoints/reversal and diagnostic rendering. The screens build, rendering checks, all 40 relevant settings/documentation tests and changed-file Ruff pass. Installed from source at `f162018` with the wearer's approval to restart only the Frametop desktop; the running binary matches the build. Visual wearer review remains pending.

An additional **Native-like** style offers gray-to-almost-white hover feedback, slightly longer/thicker arms and a 3 mm outward shift without changing the target. **Corner roundness** is configurable for both outline styles. These are approximations of the wearer's native-window reference, not reused native controls. Rendering tests cover both styles, square/intermediate/round bends, hover growth/shift and canvas bounds.

Follow-up: the sharp outline defaults to a 2 cm clear gap. Gap, actual laser hover-target size and approach-preview radius are independently configurable. Actual hover/drag uses a distinct configurable color, full configured opacity and a 60% thicker sharp outline, rather than only the approach fade. Native controls and near-hand reach remain unchanged. Rendering checks include gap clearance at 0/2/10 cm and distinct hover styling; the screens build and all 39 relevant settings/documentation tests pass. Installed from source at `c3906b6`, with the wearer's approval to restart only the Frametop desktop; the running binary matches the build. Visual wearer review remains pending.

Replaces Frametop's diffuse corner halo with a sharp, antialiased rounded outline. The indicator still follows any of the four corners; the resize interaction is unchanged. **Display Settings → Visibility & pins → Resize corners** offers sharp/original-glow styles, color, opacity, and sharp-outline arm length and thickness. See [Floating windows](floating-windows.md) for keys and defaults. The screens component builds; rendering checks cover all four orientations, extreme dimensions, antialiasing and a clear panel interior. All 39 relevant settings/documentation tests and changed-file Ruff pass. Local screenshot evidence stays private pending review; headset validation is pending.

## Native Konsole from the dashboard

`DASHBOARD_NATIVE_APPS='konsole'` leaves Konsole picked from SteamVR's **+** menu on SteamVR's original/native launch path even while other dashboard apps route into Frametop. It changes where new windows open, not their distribution: Konsole in Frametop already runs on the SteamOS host, not Fedora. The setting is checked per launch, matches exact executable basenames and preserves other apps' routing. Tests cover executable paths, simple environment wrappers, argument false positives, multiple exceptions and preserving the original command line. Existing windows are not moved. See [Floating windows](floating-windows.md) for configuration details.

## Branch workflow

- **`origin`** points to `firtoz/frametop`; it is the default push destination.
- **`main`** integrates the personal customizations and this log. Never submit it wholesale to upstream.
- **`upstream`** points to `Frametop/frametop`; its push URL is deliberately disabled.
- **`upstream-main`** tracks upstream's clean `main` and has a matching branch on the fork. Update it by fast-forwarding, not by adding personal commits.
- **PR branches** start from the clean tracking branch and contain only the relevant feature/fix. The personal README notice, this log and fork-specific agent instructions stay out of upstream PRs.

Periodically fetch upstream and update the tracking branch, then integrate those changes into personal `main`. Rebasing an already published `main` rewrites history and needs explicit approval and a guarded `--force-with-lease` push; merging is the non-rewriting alternative. After a PR merges, check whether its local commits can be dropped during the next rebase.

The generic installer/updater in the upstream README targets upstream. Do not use it as an update mechanism for this customized branch without checking what it will replace.

## Maintaining this log

For every new customization, add or update its entry, behavior summary, configuration location, upstream PR link and validation notes. Link approved screenshots where useful. Keep entries when their PR merges, marking them as integrated rather than silently losing the history. Keep fork-only documentation separate from contribution branches.
