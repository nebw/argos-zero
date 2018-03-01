//
// Copyright 2006 and onwards, Lukasz Lew
//

#ifndef VERTEX_H_
#define VERTEX_H_

#include <istream>
#include <string>
#include "config.hpp"
#include "nat.hpp"

namespace {
const static uint dNS = (board_size + 2);
const static uint dWE = 1;
}  // namespace

class Dir;

namespace Coord {
bool IsOk(int coord);

std::string RowToGtpString(uint row);
std::string ColumnToGtpString(uint column);
int RowOfGtpInt(int r);
int ColumnOfGtpChar(char c);
}  // namespace Coord





class Vertex : public Nat<Vertex> {
public:
    // Constructors.

    explicit Vertex(){};  // TODO remove it

    static Vertex Pass();
    static Vertex Any();  // NatMap doesn't work with Invalid

    static Vertex OfCoords(int row, int column);
    static inline Vertex OfCoordsUnsafe(int row, int column) {
        return Vertex::OfRaw((row + 1) * dNS + (column + 1) * dWE);
    }
    static Vertex OfGtpString(const std::string& s);
    static Vertex OfGtpStream(std::istream& in);
    static Vertex OfSgfString(const std::string& s);

    // Utilities.

    int GetRow() const;
    int GetColumn() const;


    // This can be achieved quicker by color_at lookup.
    bool IsOnBoard() const;

    inline Vertex N() const { return Vertex::OfRaw(GetRaw() - dNS); }
    inline Vertex W() const { return Vertex::OfRaw(GetRaw() - dWE); }
    inline Vertex E() const { return Vertex::OfRaw(GetRaw() + dWE); }
    inline Vertex S() const { return Vertex::OfRaw(GetRaw() + dNS); }

    inline Vertex NW() const { return N().W(); }
    inline Vertex NE() const { return N().E(); }
    inline Vertex SW() const { return S().W(); }
    inline Vertex SE() const { return S().E(); }

    Vertex Nbr(Dir d) const;

    std::string ToGtpString() const;

    // Other.

    static const uint kBound = (board_size + 2) * (board_size + 2) + 2;
    // board with guards + pass + any

private:
    friend class Nat<Vertex>;
    explicit Vertex(uint raw);
};

#define for_each_8_nbr(center_v, nbr_v, block) \
    {                                          \
        Vertex nbr_v;                          \
        nbr_v = center_v.N();                  \
        block;                                 \
        nbr_v = center_v.W();                  \
        block;                                 \
        nbr_v = center_v.E();                  \
        block;                                 \
        nbr_v = center_v.S();                  \
        block;                                 \
        nbr_v = center_v.NW();                 \
        block;                                 \
        nbr_v = center_v.NE();                 \
        block;                                 \
        nbr_v = center_v.SW();                 \
        block;                                 \
        nbr_v = center_v.SE();                 \
        block;                                 \
    }

// -----------------------------------------------------------------------------
#endif
