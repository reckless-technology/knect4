// The window drawn to a PNG with a game in a chosen state, for review images and the docs: `mid` (a game under way,
// the person's puck held), `drop` (it falling), `won` (the person's four lit, the banner shown), `lost`, `empty`,
// `friend` (two people, Yellow to move) and `friend-won` (Yellow's four), `hint` (the hint found), `emptying` (the
// pucks falling out through the slider), `marks` (the pucks' marks), `keys` (the keyboard's marker), `small`, `large`
// and `supersize` (the other boards) and `about` (the About card); and with `icon <size>` the app's icon.

#include "app/about_view.h"
#include "app/board_art.h"
#include "app/board_view.h"
#include "app/controller.h"
#include "app/main_view.h"
#include "app/theme.h"

#include "Embedded.h"
#include "SkiaCanvas.h"
#include "View.h"
#include "version.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{

using knect4::app::BoardPicture;

/** The window's host with no window: text measured as Skia draws it, so a line of text lays out as the window does. */
class RenderHost final : public reckless::view::Host
{
  public:
    /** Nothing to repaint: the whole window is drawn once. @param root_area The area. */
    void Invalidate(reckless::gfx::RectI root_area) override
    {
        static_cast<void>(root_area);
    }

    /**
     * Text measured as Skia draws it.
     * @param font The font.
     * @param text The text.
     * @return Its width.
     */
    float TextWidth(const reckless::gfx::Font &font, std::string_view text) const override
    {
        return reckless::gfx::MeasureText(font, text);
    }
};

/**
 * The image written to `path` as a PNG.
 * @param image The image.
 * @param path Where.
 * @return Whether it was written.
 */
bool Write(const reckless::gfx::Image &image, const std::string &path)
{
    const std::vector<std::uint8_t> png = reckless::gfx::EncodePng(image);
    std::ofstream                   file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(png.data()), static_cast<std::streamsize>(png.size()));
    return static_cast<bool>(file);
}

/**
 * The icon: the board's lower left corner, a red puck in its slot and a yellow one beside it, on a rounded square.
 * @param size Its side, in pixels.
 * @param path Where to write it.
 * @return The exit code.
 */
int RenderIcon(int size, const std::string &path)
{
    reckless::gfx::Image image = reckless::gfx::MakeImage(size, size);
    {
        const auto canvas = reckless::gfx::MakeImageCanvas(image, static_cast<float>(size) / 256.0f);
        if (!canvas)
        {
            return 1;
        }
        reckless::gfx::Canvas &c = *canvas;
        // Two by two slots of the board, red and yellow crosswise, cut out of a rounded square of its plastic: the
        // slots' block centred, at a scale that leaves the next slots' holes just outside the square.
        const knect4::app::art::Layout layout;
        const auto                     ColumnX = [&layout](float column) { return layout.ColumnX(column); };
        const auto                     RowY    = [&layout](int row) { return layout.RowY(static_cast<float>(row)); };
        reckless::gfx::Path            square;
        square.AddRoundedRect({8.0f, 8.0f, 240.0f, 240.0f}, 52.0f);
        c.ClipToPath(square);
        c.FillAll(knect4::app::theme::kWindow);
        const float centre_x = ColumnX(0.5f);
        const float centre_y = (RowY(0) + RowY(1)) / 2.0f;
        c.AddTransform(reckless::gfx::Affine::Translation(-centre_x, -centre_y)
                           .FollowedBy(reckless::gfx::Affine::Scale(1.4f))
                           .Translated(128.0f, 128.0f));
        knect4::app::art::DrawPuck(c, {ColumnX(0.0f), RowY(0)}, knect4::app::art::kPuckRadius,
                                   knect4::app::theme::kRed);
        knect4::app::art::DrawPuck(c, {ColumnX(1.0f), RowY(0)}, knect4::app::art::kPuckRadius,
                                   knect4::app::theme::kYellow);
        knect4::app::art::DrawPuck(c, {ColumnX(0.0f), RowY(1)}, knect4::app::art::kPuckRadius,
                                   knect4::app::theme::kYellow);
        knect4::app::art::DrawPuck(c, {ColumnX(1.0f), RowY(1)}, knect4::app::art::kPuckRadius,
                                   knect4::app::theme::kRed);
        knect4::app::art::DrawFrame(c, layout);
    }
    return Write(image, path) ? 0 : 1;
}

} // namespace

/**
 * Draw the window, or the icon, into a PNG.
 * @param argc The argument count.
 * @param argv The arguments.
 * @return 0 when the file was written, 1 otherwise, 2 for a usage error.
 */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: knect4_render <out.png> [state] [scale]\n"
                             "  state: mid drop won lost empty friend friend-won hint emptying marks keys small large\n"
                             "         supersize about\n"
                             "       knect4_render <out.png> icon <size>\n");
        return 2;
    }
    reckless::gfx::SetSerifFaces(reckless::embedded::Find("NotoSerif-Regular.ttf"),
                                 reckless::embedded::Find("NotoSerif-Bold.ttf"));
    const std::string out   = argv[1];
    const std::string state = argc > 2 ? argv[2] : "mid";
    if (state == "icon")
    {
        return RenderIcon(argc > 3 ? std::atoi(argv[3]) : 256, out);
    }
    const float scale = argc > 3 ? static_cast<float>(std::atof(argv[3])) : 1.0f;

    // The person (between two people, Red) starts every game set up here: their pucks are the odd moves.
    knect4::app::Settings settings;
    settings.is_two_player = state == "friend" || state == "friend-won";
    settings.has_marks     = state == "marks";
    if (state == "small")
    {
        settings.board = {6, 5};
    }
    else if (state == "large")
    {
        settings.board = {9, 7};
    }
    else if (state == "supersize")
    {
        settings.board = {16, 8};
    }
    if (settings.is_two_player)
    {
        settings.red_name    = "Ann";
        settings.yellow_name = "Sam";
    }
    knect4::app::Controller controller(nullptr, settings);
    if (state == "small" || state == "large" || state == "supersize")
    {
        // A few moves into a game on the board, the person's puck held over the middle.
        const int middle = settings.board.columns / 2;
        controller.PlaceMoves({middle, middle, middle - 1, middle + 1, middle + 1, 0});
        controller.Hover(middle - 1);
        controller.Frame(0.25);
    }
    else if (state == "hint" || state == "marks" || state == "keys")
    {
        // The person's turn with a four to make at 3: the hint found and shown, the marks, or the keys' marker.
        controller.PlaceMoves({0, 6, 1, 6, 2, 5});
        if (state == "hint")
        {
            controller.AskHint();
            for (int frame = 0; frame != 600 && controller.hint_column() < 0; ++frame)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                controller.Frame(1.0 / 60.0);
            }
        }
        if (state == "keys")
        {
            controller.Step(1);
        }
        else
        {
            controller.Hover(4);
        }
        controller.Frame(0.25);
    }
    else if (state == "emptying")
    {
        // The last game's pucks falling out through the slider.
        controller.PlaceMoves({3, 3, 2, 4, 4, 2, 5, 1, 3, 3, 1, 0, 6});
        controller.NewGame();
        for (int frame = 0; frame != 22; ++frame)
        {
            controller.Frame(1.0 / 60.0);
        }
    }
    else if (state == "friend")
    {
        // Two people: Yellow's turn, Yellow's puck held over column 4.
        controller.PlaceMoves({3, 3, 2, 4, 4, 2, 5, 1, 3});
        controller.Hover(4);
        controller.Frame(0.25);
    }
    else if (state == "friend-won")
    {
        // Yellow's four up the diagonal from (1,0), dropped last.
        controller.PlaceMoves({2, 1, 3, 2, 4, 4, 3, 3, 4});
        controller.Drop(4);
    }
    else if (state == "mid" || state == "drop")
    {
        // The person's turn, their puck held over column 5 - or dropped, half way down.
        controller.PlaceMoves({3, 3, 2, 4, 4, 2, 5, 1, 3, 3});
        controller.Hover(5);
        controller.Frame(0.25);
        if (state == "drop")
        {
            controller.Drop(5);
            for (int frame = 0; frame != 15; ++frame)
            {
                controller.Frame(1.0 / 60.0);
            }
        }
    }
    else if (state == "won")
    {
        // Five across the bottom, the last dropped and settled.
        controller.PlaceMoves({3, 3, 2, 2, 4, 4, 6, 6});
        controller.Drop(5);
    }
    else if (state == "lost")
    {
        controller.PlaceMoves({0, 3, 0, 3, 1, 3, 6, 3});
    }
    for (int frame = 0;
         frame != 240 && controller.phase() == knect4::app::Phase::kDropping && state != "drop" && state != "emptying";
         ++frame)
    {
        controller.Frame(1.0 / 60.0);
    }

    // The host before the view, so it outlives it: the view tells its host as it goes.
    RenderHost            host;
    knect4::app::MainView view;
    view.SetHost(&host);
    view.SetBounds({knect4::app::MainView::kDesignWidth, knect4::app::MainView::kDesignHeight});
    view.Show(controller);
    std::unique_ptr<knect4::app::AboutView> about;
    if (state == "about")
    {
        about = std::make_unique<knect4::app::AboutView>(knect4::kVersion);
        view.SetOverlay(about.get());
    }

    const int width  = static_cast<int>(static_cast<float>(knect4::app::MainView::kDesignWidth) * scale + 0.5f);
    const int height = static_cast<int>(static_cast<float>(knect4::app::MainView::kDesignHeight) * scale + 0.5f);
    reckless::gfx::Image image = reckless::gfx::MakeImage(width, height);
    {
        const auto canvas = reckless::gfx::MakeImageCanvas(image, scale);
        if (!canvas)
        {
            std::fprintf(stderr, "knect4_render: no canvas\n");
            return 1;
        }
        reckless::view::PaintTree(view, *canvas);
    }
    if (!Write(image, out))
    {
        std::fprintf(stderr, "knect4_render: could not write %s\n", out.c_str());
        return 1;
    }
    std::printf("%s: %d x %d at %.2fx\n", out.c_str(), width, height, static_cast<double>(scale));
    return 0;
}
