// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/ui/live_wallpaper.h"

#include "base/random.h"
#include "base/timer.h"
#include "ui/painter.h"
#include "window/themes/window_theme.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QSaveFile>
#include <QtGui/QGuiApplication>

#include <cmath>

namespace Nexo::LiveWallpaper {
namespace {

constexpr auto kFrame = crl::time(50); // 20 frames a second.
constexpr auto kMaxParticles = 600;
constexpr auto kMaxGrain = 2500;

struct Particles {
	QSize size;
	Effect effect = Effect::None;
	int density = 0;
	std::vector<QPointF> point;
	std::vector<float64> radius;
	std::vector<float64> speed;
	std::vector<float64> phase;
	crl::time last = 0;
};

struct State {
	Options options;
	bool loaded = false;
	std::vector<QPointer<QWidget>> widgets;
	base::flat_map<QWidget*, Particles> particles;
	std::unique_ptr<base::Timer> timer;
};

State &Store() {
	static auto result = State();
	return result;
}

[[nodiscard]] QString ConfigPath() {
	return cWorkingDir() + u"tdata/nexo/live_wallpaper.json"_q;
}

[[nodiscard]] float64 Random01() {
	return base::RandomValue<uint32>() / float64(0xFFFFFFFFU);
}

void EnsureLoaded() {
	auto &state = Store();
	if (state.loaded) {
		return;
	}
	state.loaded = true;
	auto file = QFile(ConfigPath());
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto object = QJsonDocument::fromJson(file.readAll()).object();
	state.options.effect = Effect(std::clamp(
		object.value(u"effect"_q).toInt(0),
		0,
		int(Effect::Stars)));
	state.options.density = std::clamp(
		object.value(u"density"_q).toInt(50),
		10,
		100);
	state.options.speed = std::clamp(
		object.value(u"speed"_q).toInt(100),
		25,
		200);
}

void Tick() {
	auto &state = Store();
	const auto active = QGuiApplication::applicationState()
		== Qt::ApplicationActive;
	auto any = false;
	for (auto i = state.widgets.begin(); i != state.widgets.end();) {
		const auto widget = i->data();
		if (!widget) {
			i = state.widgets.erase(i);
			continue;
		}
		if (active && widget->isVisible() && !widget->window()->isMinimized()) {
			widget->update();
			any = true;
		}
		++i;
	}
	if (state.widgets.empty() || state.options.effect == Effect::None) {
		state.timer->cancel();
	} else if (!any) {
		// Window hidden or inactive: check again rarely, draw nothing.
		state.timer->callOnce(500);
	} else {
		state.timer->callOnce(kFrame);
	}
}

void Track(not_null<QWidget*> widget) {
	auto &state = Store();
	const auto i = ranges::find(state.widgets, widget.get(), [](
			const QPointer<QWidget> &value) {
		return value.data();
	});
	if (i == state.widgets.end()) {
		state.widgets.push_back(widget.get());
		QObject::connect(widget, &QObject::destroyed, [=] {
			Store().particles.remove(widget.get());
		});
	}
	if (!state.timer) {
		state.timer = std::make_unique<base::Timer>(Tick);
	}
	if (!state.timer->isActive()) {
		state.timer->callOnce(kFrame);
	}
}

void Setup(Particles &list, QSize size, const Options &options) {
	list = Particles();
	list.size = size;
	list.effect = options.effect;
	list.density = options.density;
	const auto area = (size.width() / 100.) * (size.height() / 100.);
	const auto scale = options.density / 50.;
	auto count = 0;
	switch (options.effect) {
	case Effect::Rain: count = int(area * 4 * scale); break;
	case Effect::Snow: count = int(area * 3 * scale); break;
	case Effect::Grain: count = int(area * 30 * scale); break;
	case Effect::Stars: count = int(area * 2 * scale); break;
	case Effect::None: break;
	}
	count = std::clamp(
		count,
		0,
		(options.effect == Effect::Grain) ? kMaxGrain : kMaxParticles);
	list.point.resize(count);
	list.radius.resize(count);
	list.speed.resize(count);
	list.phase.resize(count);
	for (auto i = 0; i != count; ++i) {
		list.point[i] = QPointF(
			Random01() * size.width(),
			Random01() * size.height());
		list.phase[i] = Random01() * 6.283;
		switch (options.effect) {
		case Effect::Rain:
			list.radius[i] = 10 + Random01() * 18;
			list.speed[i] = 500 + Random01() * 400;
			break;
		case Effect::Snow:
			list.radius[i] = 1.2 + Random01() * 2.6;
			list.speed[i] = 25 + Random01() * 45;
			break;
		default:
			list.radius[i] = 0.6 + Random01() * 1.2;
			list.speed[i] = 0.6 + Random01() * 1.8;
			break;
		}
	}
}

} // namespace

Options CurrentOptions() {
	EnsureLoaded();
	return Store().options;
}

void SetOptions(const Options &options) {
	EnsureLoaded();
	auto &state = Store();
	state.options = options;
	state.options.density = std::clamp(options.density, 10, 100);
	state.options.speed = std::clamp(options.speed, 25, 200);
	state.particles.clear();

	auto object = QJsonObject();
	object.insert(u"effect"_q, int(state.options.effect));
	object.insert(u"density"_q, state.options.density);
	object.insert(u"speed"_q, state.options.speed);
	auto file = QSaveFile(ConfigPath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
		file.commit();
	}
	for (const auto &widget : state.widgets) {
		if (widget) {
			widget->update();
		}
	}
	if (state.timer && state.options.effect != Effect::None) {
		state.timer->callOnce(kFrame);
	}
}

void Paint(not_null<QWidget*> widget, QPainter &p, QRect rect) {
	const auto options = CurrentOptions();
	if (options.effect == Effect::None || rect.isEmpty()) {
		return;
	}
	Track(widget);
	auto &list = Store().particles[widget.get()];
	if (list.size != rect.size()
		|| list.effect != options.effect
		|| list.density != options.density) {
		Setup(list, rect.size(), options);
	}
	const auto now = crl::now();
	const auto dt = list.last
		? std::min((now - list.last) / 1000., 0.1) * (options.speed / 100.)
		: 0.;
	list.last = now;

	const auto dark = Window::Theme::IsNightMode();
	auto color = dark ? QColor(255, 255, 255) : QColor(0, 0, 0);
	const auto w = rect.width();
	const auto h = rect.height();
	const auto t = now / 1000.;

	auto hq = PainterHighQualityEnabler(p);
	p.save();
	p.setClipRect(rect);
	p.translate(rect.topLeft());
	switch (options.effect) {
	case Effect::Rain: {
		constexpr auto kSlant = 0.18;
		for (auto i = 0; i != int(list.point.size()); ++i) {
			auto &point = list.point[i];
			const auto length = list.radius[i];
			point.ry() += list.speed[i] * dt;
			point.rx() += list.speed[i] * dt * kSlant;
			if (point.y() - length > h || point.x() > w + 20) {
				point = QPointF(Random01() * (w + 40) - 40, -length - Random01() * 40);
			}
			color.setAlphaF(0.14 + (length / 28.) * 0.18);
			p.setPen(QPen(color, 1.2, Qt::SolidLine, Qt::RoundCap));
			p.drawLine(point, point - QPointF(length * kSlant, length));
		}
	} break;
	case Effect::Snow: {
		p.setPen(Qt::NoPen);
		for (auto i = 0; i != int(list.point.size()); ++i) {
			auto &point = list.point[i];
			const auto radius = list.radius[i];
			point.ry() += list.speed[i] * dt;
			if (point.y() - radius > h) {
				point = QPointF(Random01() * w, -radius);
			}
			const auto drift = std::sin(t * 0.8 + list.phase[i]) * 14.;
			color.setAlphaF(0.3 + (radius / 3.8) * 0.3);
			p.setBrush(color);
			p.drawEllipse(QPointF(point.x() + drift, point.y()), radius, radius);
		}
	} break;
	case Effect::Grain: {
		p.setPen(Qt::NoPen);
		for (auto i = 0; i != int(list.point.size()); ++i) {
			color.setAlphaF(0.04 + Random01() * 0.1);
			p.setBrush(color);
			const auto radius = list.radius[i] * 0.6;
			p.drawEllipse(
				QPointF(Random01() * w, Random01() * h),
				radius,
				radius);
		}
	} break;
	case Effect::Stars: {
		p.setPen(Qt::NoPen);
		for (auto i = 0; i != int(list.point.size()); ++i) {
			const auto twinkle = 0.5
				+ 0.5 * std::sin(t * list.speed[i] + list.phase[i]);
			color.setAlphaF(0.1 + twinkle * 0.5);
			p.setBrush(color);
			const auto radius = list.radius[i] * (0.7 + twinkle * 0.5);
			p.drawEllipse(list.point[i], radius, radius);
		}
	} break;
	case Effect::None: break;
	}
	p.restore();
}

} // namespace Nexo::LiveWallpaper
