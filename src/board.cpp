/***********************************************************************
 *
 * Copyright (C) 2009, 2013, 2014 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "board.h"

#include "cell.h"
#include "letter.h"
#include "pattern.h"
#include "word.h"

#include <QLocale>
#include <QSettings>
#include <QTimer>

Board::Board(QObject* parent)
: QGraphicsScene(parent), m_pattern(0), m_current_word(0), m_hint(0), m_finished(true), m_paused(false) {
	QTimer* auto_save = new QTimer(this);
	auto_save->setInterval(30000);
	connect(auto_save, &QTimer::timeout, this, &Board::saveGame);
	auto_save->start();

	m_wordlist = new WordList(this);
}

//-----------------------------------------------------------------------------

Board::~Board() {
	cleanUp();
}

//-----------------------------------------------------------------------------

void Board::check(const QString& original_word, const QString& current_word) {
	emit wordSolved(original_word, current_word);
	m_finished = true;
	for (Word* word : m_words) {
		m_finished &= word->isCorrect();
	}
	if (m_finished) {
		QSettings().remove("Current/Words");
		emit finished();
	}
}

//-----------------------------------------------------------------------------

void Board::click(const QString& word) {
	emit wordSelected(word);
}

//-----------------------------------------------------------------------------

void Board::setCurrentWord(Word* word) {
	if (m_current_word) {
		m_current_word->setHighlight(false);
	}
	m_current_word = !m_paused ? word : 0;
	if (m_current_word) {
		m_current_word->setHighlight(true);
	}
	delete m_hint;
	m_hint = 0;
	emit hintAvailable(m_current_word != 0);
}

//-----------------------------------------------------------------------------

void Board::setPaused(bool paused) {
	if (m_finished) {
		return;
	}

	m_paused = paused;
	for (int x = 0; x < m_cells.count(); ++x) {
		QList<Cell*>& cells = m_cells[x];
		for (int y = 0; y < cells.count(); ++y) {
			Cell* cell = cells[y];
			if (cell) {
				cell->letter()->setPaused(paused);
			}
		}
	}
	if (m_paused) {
		setCurrentWord(0);
	}
	emit pauseChanged();
}

//-----------------------------------------------------------------------------

void Board::openGame() {
	cleanUp();
	emit loading();

	QSettings settings;
	m_wordlist->setLanguage(settings.value("Current/Language", WordList::defaultLanguage()).toString());
	m_pattern = Pattern::create(m_wordlist, settings.value("Current/Pattern").toInt());
	m_pattern->setCount(settings.value("Current/Count").toInt());
	m_pattern->setLength(settings.value("Current/Length").toInt());
	m_pattern->setSeed(settings.value("Current/Seed").toUInt());

	connect(m_pattern, &Pattern::generated, this, &Board::patternGenerated);
	m_pattern->start();
}

//-----------------------------------------------------------------------------

bool Board::openGame(const QString& number) {
	if (!number.startsWith("4")) {
		return false;
	}

	// Parse language
	int index = 1;
	for (int i = 1; i < number.length(); ++i) {
		if (!number.at(i).isDigit()) {
			index = i;
		} else {
			break;
		}
	}
	QString language = WordList::defaultLanguage();
	if (index > 1) {
		language = number.mid(1, index);
		index += 1;
	}

	// Parse type
	if (index == number.length()) {
		return false;
	}
	int pattern = number.at(index).digitValue();

	// Parse count
	if (++index == number.length()) {
		return false;
	}
	int count = number.at(index).digitValue();

	// Parse length
	if (++index == number.length()) {
		return false;
	}
	bool ok = false;
	int length = number.mid(index, 2).toInt(&ok, 16) + 5;
	if (!ok) {
		return false;
	}

	// Parse seed
	if ((index += 2) == number.length()) {
		return false;
	}
	unsigned int seed = number.mid(index).toUInt(&ok, 16);
	if (!ok) {
		return false;
	}

	// Start game
	cleanUp();
	emit loading();

	m_wordlist->setLanguage(language);
	m_pattern = Pattern::create(m_wordlist, pattern);
	m_pattern->setCount(count);
	m_pattern->setLength(length);
	m_pattern->setSeed(seed);

	connect(m_pattern, &Pattern::generated, this, &Board::patternGenerated);
	m_pattern->start();

	return true;
}

//-----------------------------------------------------------------------------

void Board::saveGame() {
	if (!m_finished && !m_words.isEmpty()) {
		QStringList words;
		for (Word* word : m_words) {
			words.append(word->toString());
		}
		QSettings().setValue("Current/Words", words);
	}
}

//-----------------------------------------------------------------------------

void Board::showHint() {
	if (m_current_word && !mouseGrabberItem()) {
		delete m_hint;
		m_hint = m_current_word->hint();
	}
}

//-----------------------------------------------------------------------------

void Board::togglePaused() {
	setPaused(!m_paused);
}

//-----------------------------------------------------------------------------

void Board::patternGenerated() {
	m_words = m_pattern->solution();
	QSize size = m_pattern->size();
	setSceneRect(0, 0, size.width() * 34 + 2, size.height() * 34 + 34);

	for (int x = 0; x < size.width(); ++x) {
		QList<Cell*> cells;
		for (int y = 0; y < size.height(); ++y) {
			cells.append(0);
		}
		m_cells.append(cells);
	}

	for (Word* word : m_words) {
		word->setBoard(this);
		QList<QPoint> positions = word->positions();
		for (int i = 0; i < positions.count(); ++i) {
			const QPoint& pos = positions.at(i);
			Cell* cell = m_cells.at(pos.x()).at(pos.y());
			if (cell == 0) {
				Letter* letter = new Letter(word->at(i), this);
				addItem(letter);

				cell = new Cell(pos, letter);
				cell->setWord(word);
				m_cells[pos.x()][pos.y()] = cell;
			} else {
				cell->letter()->setJoin();
				cell->setWord(0);
			}
		}
	}

	for (Word* word : m_words) {
		emit wordAdded(word->toString());
		word->shuffle(m_pattern->words());
	}

	QStringList previous = QSettings().value("Current/Words").toStringList();
	if (previous.count() == m_words.count()) {
		for (int i = 0; i < m_words.count(); ++i) {
			m_words[i]->fromString(previous.at(i));
		}
	}
	saveGame();

	emit started();
}

//-----------------------------------------------------------------------------

void Board::cleanUp() {
	delete m_pattern;
	m_pattern = 0;
	clear();
	for (int i = 0; i < m_cells.count(); ++i) {
		qDeleteAll(m_cells[i]);
	}
	m_cells.clear();
	m_words.clear();
	m_current_word = 0;
	m_hint = 0;
	m_finished = false;
	m_paused = false;
}
