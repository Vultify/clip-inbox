#pragma once

#include "clip.hpp"

#include <QFrame>

// One clip in the inbox: what kind it is and its file name.
class ClipRow : public QFrame {
	Q_OBJECT

public:
	explicit ClipRow(const Clip &clip, QWidget *parent = nullptr);
};
