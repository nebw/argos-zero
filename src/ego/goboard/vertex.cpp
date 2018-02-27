//
// Copyright 2006 and onwards, Lukasz Lew
//

#include "to_string.hpp"

#include "dir.hpp"
#include "vertex.hpp"

// -----------------------------------------------------------------------------

namespace Coord {
const string col_tab = "ABCDEFGHJKLMNOPQRSTUVWXYZ";

bool IsOk(int coord) { return static_cast<uint>(coord) < board_size; }

string RowToGtpString(uint row) {
    EGO_CHECK(row < board_size);
    return ToString(board_size - row);
}

string ColumnToGtpString(uint column) {
    EGO_CHECK(column < board_size);
    return ToString(col_tab[column]);
}

int RowOfGtpInt(int r) { return board_size - r; }

int ColumnOfGtpChar(char c) { return col_tab.find(c); }
}  // namespace Coord

//--------------------------------------------------------------------------------

Vertex::Vertex(uint raw) : Nat<Vertex>(raw) {}

Vertex Vertex::Pass() { return Vertex(kBound - 2); }

Vertex Vertex::Any() { return Vertex(kBound - 1); }

Vertex Vertex::OfCoords(int row, int column) {
    if (!Coord::IsOk(row) || !Coord::IsOk(column)) { return Vertex::Invalid(); }
    return Vertex::OfRaw((row + 1) * dNS + (column + 1) * dWE);
}

Vertex Vertex::OfSgfString(const string& s) {
    if (s == "" || (s == "tt" && board_size <= 19)) return Pass();
    if (s.size() != 2) return Invalid();
    int col = s[0] - 'a';
    int row = s[1] - 'a';
    return Vertex::OfCoords(row, col);
}

Vertex Vertex::OfGtpString(const string& s) {
    if (s == "pass" || s == "PASS" || s == "Pass") return Pass();

    istringstream parser(s);
    char c;
    int r;
    if (!(parser >> c >> r)) return Invalid();

    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    int row = Coord::RowOfGtpInt(r);
    int col = Coord::ColumnOfGtpChar(c);

    return Vertex::OfCoords(row, col);
}

Vertex Vertex::OfGtpStream(istream& in) {
    string s;
    in >> s;
    if (!in) return Invalid();
    Vertex v = OfGtpString(s);
    if (v == Invalid()) in.setstate(ios_base::badbit);
    return v;
}

int Vertex::GetRow() const { return int(GetRaw() / dNS - 1); }

int Vertex::GetColumn() const { return int(GetRaw() % dNS - 1); }


bool Vertex::IsOnBoard() const { return Coord::IsOk(GetRow()) & Coord::IsOk(GetColumn()); }

Vertex Vertex::Nbr(Dir d) const {
    ASSERT(IsOnBoard());
    const static uint delta_raw[8] = {
        // TODO NatMap<Dir, uint> when compiler allows
        -dNS, +dWE, +dNS, -dWE, -dNS - dWE, -dNS + dWE, +dNS + dWE, +dNS - dWE};

    return OfRaw(GetRaw() + delta_raw[d.GetRaw()]);
}

string Vertex::ToGtpString() const {
    if (*this == Invalid()) return "invalid";
    if (*this == Pass()) return "pass";
    if (*this == Any()) return "any";
    if (!IsOnBoard()) return "off board";

    return Coord::ColumnToGtpString(GetColumn()) + Coord::RowToGtpString(GetRow());
}