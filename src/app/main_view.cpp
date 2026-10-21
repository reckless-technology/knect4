#include "app/main_view.h"

#include "app/theme.h"

#include <utility>

namespace connect4::app
{

MainView::MainView()
{
    SetOpaque(true);
    AddChild(board_);
    AddChild(panel_);
}

void MainView::SetMenuBarView(std::unique_ptr<reckless::app::MenuBarView> bar)
{
    if (menu_bar_)
    {
        RemoveChild(*menu_bar_);
    }
    menu_bar_ = std::move(bar);
    if (menu_bar_)
    {
        AddChild(*menu_bar_);
    }
    Resized();
}

void MainView::Show(const Controller &controller)
{
    board_.SetPicture(controller.Picture());
    panel_.Show(controller);
}

void MainView::Paint(reckless::gfx::Canvas &c)
{
    // The window's colour, a little lighter where the board stands, as a lamp over a table.
    c.FillAll(theme::kWindow);
    const auto  area  = board_.bounds().ToFloat();
    const float reach = std::max(area.width(), area.height()) * 0.62f;
    c.SetGradient(reckless::gfx::Gradient{theme::kWindow.Brighter(0.07f),
                                          {area.CentreX(), area.CentreY() - 40.0f},
                                          theme::kWindow.Darker(0.25f),
                                          {area.CentreX() + reach, area.CentreY() - 40.0f},
                                          true});
    c.FillRect(area);
}

void MainView::Resized()
{
    int top = 0;
    if (menu_bar_)
    {
        const int bar_height = menu_bar_->PreferredHeight();
        menu_bar_->SetBounds({0, 0, width(), bar_height});
        top = bar_height;
    }
    const int panel_width = SidePanel::kWidth;
    board_.SetBounds({0, top, std::max(width() - panel_width, 0), std::max(height() - top, 0)});
    panel_.SetBounds({width() - panel_width, top, panel_width, std::max(height() - top, 0)});
}

} // namespace connect4::app
