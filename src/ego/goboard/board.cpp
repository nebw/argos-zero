//
// Copyright 2006 and onwards, Lukasz Lew
//

#include <cmath>
#include <cstring>
#include <iostream>
#include <set>

#include "board.hpp"
#include "fast_stack.hpp"

// TODO    center_v.check_is_on_board ();
#define vertex_for_each_4_nbr(center_v, nbr_v, block) \
    {                                                 \
        Vertex nbr_v;                                 \
        nbr_v = center_v.N();                         \
        block;                                        \
        nbr_v = center_v.W();                         \
        block;                                        \
        nbr_v = center_v.E();                         \
        block;                                        \
        nbr_v = center_v.S();                         \
        block;                                        \
    }

// TODO center_v.check_is_on_board ();
#define vertex_for_each_diag_nbr(center_v, nbr_v, block) \
    {                                                    \
        Vertex nbr_v;                                    \
        nbr_v = center_v.NW();                           \
        block;                                           \
        nbr_v = center_v.NE();                           \
        block;                                           \
        nbr_v = center_v.SW();                           \
        block;                                           \
        nbr_v = center_v.SE();                           \
        block;                                           \
    }

#define FOREACH_DIR(d, block) \
    {                         \
        Dir d;                \
        d = Dir::N();         \
        block;                \
        d = Dir::E();         \
        block;                \
        d = Dir::S();         \
        block;                \
        d = Dir::W();         \
        block;                \
        d = Dir::NW();        \
        block;                \
        d = Dir::NE();        \
        block;                \
        d = Dir::SE();        \
        block;                \
        d = Dir::SW();        \
        block;                \
    }

RawBoard::NbrCounter RawBoard::NbrCounter::OfCounts(uint black_cnt, uint white_cnt,
                                                    uint empty_cnt) {
    ASSERT(black_cnt <= max);
    ASSERT(white_cnt <= max);
    ASSERT(empty_cnt <= max);
    RawBoard::NbrCounter nc;
    nc.bitfield = (black_cnt << f_shift[0]) + (white_cnt << f_shift[1]) + (empty_cnt << f_shift[2]);
    return nc;
}

RawBoard::NbrCounter RawBoard::NbrCounter::Empty() { return OfCounts(0, 0, max); }

void RawBoard::NbrCounter::player_inc(Player player) {
    bitfield += player_inc_tab[player.GetRaw()];
}

void RawBoard::NbrCounter::player_dec(Player player) {
    bitfield -= player_inc_tab[player.GetRaw()];
}

void RawBoard::NbrCounter::off_board_inc() {
    static const uint off_board_inc_val = (1 << f_shift[0]) + (1 << f_shift[1]) - (1 << f_shift[2]);
    bitfield += off_board_inc_val;
}

uint RawBoard::NbrCounter::empty_cnt() const { return bitfield >> f_shift[2]; }

uint RawBoard::NbrCounter::player_cnt(Player pl) const {
    static const uint f_mask = (1 << f_size) - 1;
    return (bitfield >> f_shift[pl.GetRaw()]) & f_mask;
}

uint RawBoard::NbrCounter::player_cnt_is_max(Player pl) const {
    return (player_cnt_is_max_mask[pl.GetRaw()] & bitfield) == player_cnt_is_max_mask[pl.GetRaw()];
}

void RawBoard::NbrCounter::check() const {
    if (!kCheckAsserts) return;
    ASSERT(empty_cnt() <= max);
    ASSERT(player_cnt(Player::Black()) <= max);
    ASSERT(player_cnt(Player::White()) <= max);
}

void RawBoard::NbrCounter::check(const NatMap<Color, uint>& nbr_color_cnt) const {
    if (!kCheckAsserts) return;

    uint expected_nbr_cnt =  // definition of nbr_cnt[v]
        +((nbr_color_cnt[Color::Black()] + nbr_color_cnt[Color::OffBoard()]) << f_shift[0]) +
        ((nbr_color_cnt[Color::White()] + nbr_color_cnt[Color::OffBoard()]) << f_shift[1]) +
        ((nbr_color_cnt[Color::Empty()]) << f_shift[2]);
    ASSERT(bitfield == expected_nbr_cnt);
}

const uint RawBoard::NbrCounter::max = 4;     // maximal number of neighbours
const uint RawBoard::NbrCounter::f_size = 4;  // size in bits of each of 3 counters
const uint RawBoard::NbrCounter::f_shift[3] = {
    0 * f_size,
    1 * f_size,
    2 * f_size,
};

const uint RawBoard::NbrCounter::player_cnt_is_max_mask[Player::kBound] = {  // TODO player_Map
    (max << f_shift[0]), (max << f_shift[1])};

const uint RawBoard::NbrCounter::player_inc_tab[Player::kBound] = {
    (1u << f_shift[0]) - (1u << f_shift[2]),
    (1u << f_shift[1]) - (1u << f_shift[2]),
};

// -----------------------------------------------------------------------------

namespace {
struct Precomputed {
    Precomputed() { ForEachNat(Vertex, v) square[v] = v.GetRaw() * v.GetRaw(); }
    NatMap<Vertex, uint> square;
} const static precomputed;
}  // namespace

void RawBoard::Chain::ResetOffBoard() {
    lib_cnt = 2;  // this is needed to not try to remove offboard guards
    lib_sum = 1;
    lib_sum2 = 1;
    size = 100;
    atari_v = Vertex::Any();
}

void RawBoard::Chain::Reset() {
    lib_cnt = 0;
    lib_sum = 0;
    lib_sum2 = 0;
    size = 1;
    atari_v = Vertex::Any();
}

void RawBoard::Chain::AddLib(Vertex v) {
    lib_cnt += 1;
    lib_sum += v.GetRaw();
    lib_sum2 += precomputed.square[v];
}

void RawBoard::Chain::SubLib(Vertex v) {
    lib_cnt -= 1;
    lib_sum -= v.GetRaw();
    lib_sum2 -= precomputed.square[v];
}

void RawBoard::Chain::Merge(const RawBoard::Chain& other) {
    lib_cnt += other.lib_cnt;
    lib_sum += other.lib_sum;
    lib_sum2 += other.lib_sum2;
    size += other.size;
    atari_v = Vertex::Any();
}

bool RawBoard::Chain::IsCaptured() const { return lib_cnt == 0; }

Vertex RawBoard::Chain::AtariVertex() const {
    EGO_CHECK(lib_sum % lib_cnt == 0);
    return Vertex::OfRaw(lib_sum / lib_cnt);  // TODO inefficient
}

// -----------------------------------------------------------------------------

string RawBoard::ToAsciiArt(Vertex mark_v) const {
    ostringstream out;

#define coord_for_each(rc) for (int rc = 0; rc < int(board_size); rc += 1)
#define os(n) out << " " << n
#define o_left(n) out << "(" << n
#define o_right(n) out << ")" << n

    out << " ";
    if (board_size < 10)
        out << " ";
    else
        out << "  ";
    coord_for_each(col) os(Coord::ColumnToGtpString(col));
    out << endl;

    coord_for_each(row) {
        if (board_size >= 10 && board_size - row < 10) out << " ";
        os(Coord::RowToGtpString(row));
        coord_for_each(col) {
            Vertex v = Vertex::OfCoords(row, col);
            char ch = color_at[v].ToShowboardChar();
            if (v == mark_v)
                o_left(ch);
            else if (v == mark_v.E())
                o_right(ch);
            else
                os(ch);
        }
        if (board_size >= 10 && board_size - row < 10) out << " ";
        os(Coord::RowToGtpString(row));
        out << endl;
    }

    if (board_size < 10)
        out << "  ";
    else
        out << "   ";
    coord_for_each(col) os(Coord::ColumnToGtpString(col));
    out << endl;

#undef coord_for_each
#undef os
#undef o_left
#undef o_right

    return out.str();
}

string RawBoard::ToAsciiArtInfluence(NatMap<Vertex, double> const& influence,
                                     double threshold) const {
    ostringstream out;

#define coord_for_each(rc) for (int rc = 0; rc < int(board_size); rc += 1)
#define os(n) out << " " << n
#define o_left(n) out << "(" << n
#define o_right(n) out << ")" << n

    out << " ";
    if (board_size < 10)
        out << " ";
    else
        out << "  ";
    coord_for_each(col) os(Coord::ColumnToGtpString(col));
    out << endl;

    coord_for_each(row) {
        if (board_size >= 10 && board_size - row < 10) out << " ";
        os(Coord::RowToGtpString(row));
        coord_for_each(col) {
            Vertex v = Vertex::OfCoords(row, col);
            char ch;
            if (!v.IsOnBoard())
                ch = '$';
            else if (influence[v] > threshold)
                ch = '#';
            else if (influence[v] < -threshold)
                ch = 'O';
            else
                ch = '.';
            os(ch);
        }
        if (board_size >= 10 && board_size - row < 10) out << " ";
        os(Coord::RowToGtpString(row));
        out << endl;
    }

    if (board_size < 10)
        out << "  ";
    else
        out << "   ";
    coord_for_each(col) os(Coord::ColumnToGtpString(col));
    out << endl;

#undef coord_for_each
#undef os
#undef o_left
#undef o_right

    return out.str();
}

void RawBoard::Dump() const { Dump1(LastVertex()); }

void RawBoard::Dump1(Vertex v) const {
    cerr << ToAsciiArt(v);
    cerr << ActPlayer().ToGtpString() << " to play" << endl;
}

void RawBoard::Clear() {
    empty_v_cnt = 0;
    ForEachNat(Player, pl) {
        player_v_cnt[pl] = 0;
        last_play[pl] = Vertex::Any();
    }
    move_no = 0;
    last_player = Player::White();  // act player is other
    last_ko_v = Vertex::Any();
    ko_v = Vertex::Any();
    ForEachNat(Vertex, v) {
        color_at[v] = Color::OffBoard();
        play_count[v] = 0;
        nbr_cnt[v] = NbrCounter::Empty();
        chain_next_v[v] = v;
        chain_id[v] = v;  // TODO is it needed, is it used?
        chain[v].ResetOffBoard();

        if (v.IsOnBoard()) {
            color_at[v] = Color::Empty();
            empty_pos[v] = empty_v_cnt;
            empty_v[empty_v_cnt++] = v;

            vertex_for_each_4_nbr(v, nbr_v, {
                if (!nbr_v.IsOnBoard()) { nbr_cnt[v].off_board_inc(); }
            });
        }
    }

    ForEachNat(Vertex, v) { hash3x3[v] = Hash3x3::OfBoard(color_at, v); }

    hash = recalc_hash();

    check();
}

uint RawBoard::PseudoLiberties(Vertex v) const {
    check();
    return color_at[v] == Color::Empty() ? 0 : chain_at(v).lib_cnt;
}

bool RawBoard::CheckLadderEscape(Vertex v) const {
    if (color_at[v] == Color::Empty()) {
        vertex_for_each_4_nbr(v, nbr_v, {
            if (nbr_v.IsOnBoard() && color_at[nbr_v] == Color::OfPlayer(ActPlayer())) {
                if (CheckLadderEscape(chain_at(nbr_v), v)) { return true; }
            }
        });
    }
    return false;
}

bool RawBoard::CheckLadderCapture(Vertex v) const {
    if (color_at[v] == Color::Empty()) {
        vertex_for_each_4_nbr(v, nbr_v, {
            if (nbr_v.IsOnBoard() && color_at[nbr_v] == Color::OfPlayer(ActPlayer().Other()) &&
                chain_at(nbr_v).lib_cnt == 2 && !chain_at(nbr_v).IsInAtari() &&
                IsLegal(ActPlayer(), v)) {
                RawBoard copy;
                copy.Load(*this);
                copy.PlayLegal(copy.ActPlayer(), v);
                if (!copy.CheckLadderAtVertexHasEscape(nbr_v)) { return true; }
            }
        });
    }
    return false;
}

bool RawBoard::CheckLadderEscape(const RawBoard::Chain& chain, Vertex v) const {
    if (chain.IsInAtari()) {
        if (!v.IsValid()) { return false; }

        const Vertex atariVertex = chain.AtariVertex();
        ASSERT(atariVertex == v);

        RawBoard copy;
        copy.Load(*this);

        if (IsLegal(ActPlayer(), atariVertex)) {
            copy.PlayLegal(ActPlayer(), atariVertex);

            // TODO: maybe use more efficient datastructure here
            std::vector<Vertex> emptyNeighbors;
            Vertex currentVertex = atariVertex;
            vertex_for_each_4_nbr(currentVertex, nbr_v, {
                if (nbr_v.IsOnBoard() && copy.ColorAt(nbr_v) == Color::OfPlayer(copy.ActPlayer())) {
                    // successful ladder escape if opponent group is in atari now
                    if (copy.AtariVertexOf(nbr_v).IsOnBoard()) { return true; }
                }
                if (nbr_v.IsOnBoard() && (ColorAt(nbr_v) == Color::OfPlayer(ActPlayer()))) {
                    // successful ladder escape if merge with another group and merged group has
                    // more than two liberties
                    if (copy.ChainAt(nbr_v).size > ChainAt(nbr_v).size + 1) {
                        if (!copy.AtariVertexOf(nbr_v).IsOnBoard()) {
                            // if merged group has exactly two liberties we need to check if either
                            // of them leads to a successful ladder escape
                            if (copy.chain_at(nbr_v).lib_cnt == 2) {
                                return copy.CheckLadderAtVertexHasEscape(nbr_v);
                            } else {
                                return true;
                            }
                        }
                    }
                }
                if (nbr_v.IsOnBoard() && copy.ColorAt(nbr_v) == Color::Empty()) {
                    emptyNeighbors.push_back(nbr_v);
                }
            });

            // successful ladder escape if group has more than two liberties after move
            if (emptyNeighbors.size() > 2) {
                return true;
                // if the group has exactly two liberties after move, we have to recursvly check
                // both possible opponent moves
            } else if (emptyNeighbors.size() == 2) {
                auto checkRecursive = [](RawBoard const& boardCopy, Vertex v1, Vertex v2) {
                    RawBoard neighborCopy;
                    neighborCopy.Load(boardCopy);

                    if (neighborCopy.IsLegal(Move(neighborCopy.ActPlayer(), v1))) {
                        neighborCopy.PlayLegal(neighborCopy.ActPlayer(), v1);
                        return neighborCopy.CheckLadderEscape(v2);
                        // if opponent can not fill liberty, he can not capture the group (for now)
                    } else {
                        return true;
                    }
                };

                // successful ladder escape if there's a successful escape after both possible
                // opponent moves
                if (checkRecursive(copy, emptyNeighbors[0], emptyNeighbors[1]) &&
                    checkRecursive(copy, emptyNeighbors[1], emptyNeighbors[0])) {
                    return true;
                }
            } else if (emptyNeighbors.size() < 2) {
                // unsuccessful ladder escape if group is still in atari after opponent move. this
                // situation can occur if there are neighboring stones of the same colors, but the
                // merged group is still in atari
                if (copy.AtariVertexOf(v).IsOnBoard()) {
                    return false;
                    // if the group is not in atari after the opponent move the ladder escape was
                    // successful
                } else {
                    return true;
                }
            }
        }
    }

    return false;
}

/* Expensive! */
bool RawBoard::CheckLadderAtVertexHasEscape(Vertex v) const {
    ASSERT(ChainAt(v).lib_cnt == 2);

    Vertex liberties[2];
    uint8_t libertyIdx = 0;

    Vertex act_v = v;
    do {
        vertex_for_each_4_nbr(act_v, nbr_v, {
            if (ColorAt(nbr_v) == Color::Empty()) {
                liberties[libertyIdx] = nbr_v;
                ++libertyIdx;

                if (libertyIdx == 2) { break; }
            }
        });

        act_v = chain_next_v[act_v];
    } while (act_v != v);

    auto checkEscape = [](const RawBoard* pos, Vertex chainPos, Vertex& l1, Vertex& l2) {
        RawBoard copy;
        copy.Load(*pos);
        if (l1.IsValid() && copy.IsLegal(copy.ActPlayer(), l1)) {
            copy.PlayLegal(copy.ActPlayer(), l1);
            if (!copy.CheckLadderEscape(copy.ChainAt(chainPos), l2)) { return false; }
        }
        return true;
    };

    return checkEscape(this, v, liberties[0], liberties[1]) &&
           checkEscape(this, v, liberties[1], liberties[0]);
}

Hash RawBoard::recalc_hash() const {
    Hash new_hash;

    new_hash.SetZero();

    ForEachNat(Vertex, v) {
        if (color_at[v].IsPlayer()) {
            new_hash ^= zobrist->OfPlayerVertex(color_at[v].ToPlayer(), v);
        }
    }

    return new_hash;
}

// TODO remove stupid initializers
RawBoard::RawBoard() {
    Clear();
    SetKomi(6.5);
}

uint RawBoard::PlayCount(Vertex v) const { return play_count[v]; }

uint RawBoard::EmptyVertexCount() const { return empty_v_cnt; }

uint RawBoard::Hash3x3ChangedCount() const { return hash3x3_changed.Size(); }

void RawBoard::Load(const RawBoard& save_board) {
    memcpy(this, &save_board, sizeof(RawBoard));
    check();
}

void RawBoard::SetKomi(float fkomi) { komi_inverse = int(ceil(-fkomi)); }

float RawBoard::Komi() const { return -float(komi_inverse) + 0.5; }

uint RawBoard::Size() const { return board_size; }

uint RawBoard::NumPlayerStones(Player p) const { return player_v_cnt[p]; }

Vertex RawBoard::KoVertex() const { return ko_v; }

Vertex RawBoard::LastKoVertex() const { return last_ko_v; }

Hash RawBoard::PositionalHash() const { return hash; }

Hash RawBoard::TranspositionHash() const {
    Hash newHash = hash;
    newHash ^= zobrist->OfKoVertex(KoVertex());
    newHash ^= zobrist->OfPlayer(ActPlayer());
    return newHash;
}

bool RawBoard::IsLegal(Player player, Vertex v) const {
    if (v == Vertex::Pass()) return true;
    if ((color_at[v] != Color::Empty()) | (v == ko_v)) return false;

    // check for suicide
    if (nbr_cnt[v].empty_cnt() > 0) return true;
    bool not_suicide = false;

    vertex_for_each_4_nbr(v, nbr_v, chain_at(nbr_v).lib_cnt -= 1);

    vertex_for_each_4_nbr(v, nbr_v, {
        bool atari = chain_at(nbr_v).lib_cnt == 0;
        not_suicide |=
            color_at[nbr_v].IsPlayer() & (atari != (color_at[nbr_v].ToPlayer() == player));
    });

    vertex_for_each_4_nbr(v, nbr_v, chain_at(nbr_v).lib_cnt += 1);

    return not_suicide;
}

bool RawBoard::IsLegal(Move move) const { return IsLegal(move.GetPlayer(), move.GetVertex()); }

bool RawBoard::IsEyelike(Player player, Vertex v) const {
    ASSERT(color_at[v] == Color::Empty());
    if (!nbr_cnt[v].player_cnt_is_max(player)) {
        ASSERT(!hash3x3[v].IsEyelike(player));
        return false;
    }

    NatMap<Color, int> diag_color_cnt(0);  // TODO

    vertex_for_each_diag_nbr(v, diag_v, { diag_color_cnt[color_at[diag_v]]++; });

    bool is_eye =
        diag_color_cnt[Color::OfPlayer(player.Other())] + (diag_color_cnt[Color::OffBoard()] > 0) <
        2;

    ASSERT2(hash3x3[v].IsEyelike(player) == is_eye, {
        Dump1(v);
        WW(player.ToGtpString());
        WW(is_eye);
        WW(hash3x3[v].IsEyelike(player));
        WW(hash3x3[v].ToString());
    });

    return is_eye;
}

bool RawBoard::IsEyelike(Move move) const { return IsEyelike(move.GetPlayer(), move.GetVertex()); }

Vertex RawBoard::AtariVertexOf(Vertex v) const {
    ASSERT(ColorAt(v).IsPlayer());
    return chain_at(v).atari_v;
}

Vertex RawBoard::RandomLightMove(Player pl, FastRandom& random) const {
    uint ii_start = random.GetNextUint(EmptyVertexCount());
    uint ii = ii_start;

    while (true) {  // TODO separate iterator
        Vertex v = EmptyVertex(ii);
        if (!IsEyelike(pl, v) && IsLegal(pl, v)) return v;
        ii += 1;
        ii &= ~(-(ii == EmptyVertexCount()));  // if (ii==board->empty_v_cnt) ii=0;
        if (ii == ii_start) return Vertex::Pass();
    }
}

Move RawBoard::RandomLightMove(FastRandom& random) const {
    Player pl = ActPlayer();
    return Move(pl, RandomLightMove(pl, random));
}

void RawBoard::PlayLegal(Move move) {  // TODO test with move
    PlayLegal(move.GetPlayer(), move.GetVertex());
}

    inline void
    RawBoard::PlayLegal(Player player, Vertex v) {  // TODO test with move
    check();

    tmp_vertex_set.Clear();
    hash3x3_changed.Clear();

    ASSERT(player.IsValid());
    ASSERT(v.IsValid());
    ASSERT(IsLegal(player, v));

    uint last_empty_v_cnt = empty_v_cnt;
    last_ko_v = ko_v;
    ko_v = Vertex::Any();
    last_player = player;
    last_play[player] = v;
    move_no += 1;

    if (v == Vertex::Pass()) return;

    place_stone(player, v);

    bool play_in_his_eye = nbr_cnt[v].player_cnt_is_max(player.Other());

    vertex_for_each_4_nbr(v, nbr_v, update_neighbour(v, nbr_v));

    if (play_in_his_eye && last_empty_v_cnt == empty_v_cnt) { ko_v = empty_v[empty_v_cnt - 1]; }

    // suicide support
    // if (chain_at(v).IsCaptured ()) remove_chain (v);
    ASSERT(!chain_at(v).IsCaptured());

    // covers all kinds of cases with the final string
    MaybeInAtari(v);
    check();
}

// all_inline
inline void RawBoard::update_neighbour(Vertex v, Vertex nbr_v) {
    if (!color_at[nbr_v].IsPlayer()) { return; }

    if (color_at[nbr_v] != color_at[v]) {
        if (chain_at(nbr_v).IsCaptured()) {
            remove_chain(nbr_v);
        } else {
            // reuduced liberty of nbr opponent
            MaybeInAtari(nbr_v);
        }
    } else {
        if (chain_id[nbr_v] != chain_id[v]) {
            if (chain_at(v).size > chain_at(nbr_v).size) {
                merge_chains(v, nbr_v);
            } else {
                merge_chains(nbr_v, v);
            }
        }
    }
}

// all_inline
inline void RawBoard::MaybeInAtari(Vertex v) {
    // update atari bits in hash3x3
    ASSERT2(color_at[v] != Color::Empty(), { Dump1(v); });
    if (!chain_at(v).IsInAtari()) return;

    Vertex av = chain_at(v).AtariVertex();
    ASSERT(color_at[av] == Color::Empty());

    chain_at(v).atari_v = av;
    hash3x3[av].SetAtariBits(chain_id[av.N()] == chain_id[v], chain_id[av.E()] == chain_id[v],
                             chain_id[av.S()] == chain_id[v], chain_id[av.W()] == chain_id[v]);

    if (!tmp_vertex_set.IsMarked(av)) {
        hash3x3_changed.Push(av);
        tmp_vertex_set.Mark(av);
    }
}

// all_inline
inline void RawBoard::MaybeInAtariEnd(Vertex v) {
    // update atari bits in hash3x3
    // ASSERT (color_at[v].IsPlayer());
    if (!color_at[v].IsPlayer()) return;
    if (chain_at(v).IsCaptured()) return;
    if (!chain_at(v).IsInAtari()) return;

    Vertex av = chain_at(v).AtariVertex();
    ASSERT(color_at[av] == Color::Empty());

    chain_at(v).atari_v = Vertex::Any();

    // This may not be needed, in case when atari bits were not set yet.
    // For instance the stone we play is about to be in atari, but
    // captures sth. Then chain_at(v).IsInAtari is true, but atari bits
    // are not set yet.
    hash3x3[av].UnsetAtariBits(chain_id[av.N()] == chain_id[v], chain_id[av.E()] == chain_id[v],
                               chain_id[av.S()] == chain_id[v], chain_id[av.W()] == chain_id[v]);
    if (!tmp_vertex_set.IsMarked(av)) {
        hash3x3_changed.Push(av);
        tmp_vertex_set.Mark(av);
    }
}

void RawBoard::merge_chains(Vertex v_base, Vertex v_new) {
    chain_at(v_base).Merge(chain_at(v_new));

    Vertex act_v = v_new;
    do {
        chain_id[act_v] = chain_id[v_base];
        act_v = chain_next_v[act_v];
    } while (act_v != v_new);

    swap(chain_next_v[v_base], chain_next_v[v_new]);
}

void RawBoard::remove_chain(Vertex v) {
    Color old_color = color_at[v];
    Vertex act_v = v;

    ASSERT(old_color.IsPlayer());

    // two pass chain removing

    do {
        remove_stone(act_v);
        act_v = chain_next_v[act_v];
    } while (act_v != v);

    ASSERT(act_v == v);

    do {
        vertex_for_each_4_nbr(act_v, nbr_v, {
            ASSERT(color_at[nbr_v] != old_color);
            // These two must be in this order.
            MaybeInAtariEnd(nbr_v);
            chain_at(nbr_v).AddLib(act_v);
        });

        Vertex tmp_v = act_v;
        act_v = chain_next_v[act_v];
        chain_next_v[tmp_v] = tmp_v;

    } while (act_v != v);
}

void RawBoard::place_stone(Player pl, Vertex v) {
    Color color = Color::OfPlayer(pl);
    hash ^= zobrist->OfPlayerVertex(pl, v);
    player_v_cnt[pl]++;
    color_at[v] = color;

    // TODO vector operations here would be a win.
    FOREACH_DIR(dir, {
        Vertex nbr = v.Nbr(dir);
        hash3x3[nbr].SetColorAt(dir.Opposite(), color);
        ASSERT(!tmp_vertex_set.IsMarked(nbr));
        if (color_at[nbr] == Color::Empty()) {
            hash3x3_changed.Push(nbr);
            tmp_vertex_set.Mark(nbr);
        }
    });

    play_count[v] += 1;

    empty_v_cnt--;
    empty_pos[empty_v[empty_v_cnt]] = empty_pos[v];
    empty_v[empty_pos[v]] = empty_v[empty_v_cnt];

    ASSERT(chain_next_v[v] == v);

    chain_id[v] = v;

    chain_at(v).Reset();
    vertex_for_each_4_nbr(v, nbr_v, {
        nbr_cnt[nbr_v].player_inc(pl);
        if (color_at[nbr_v] == Color::Empty()) {
            chain_at(v).AddLib(nbr_v);
        } else {
            chain_at(nbr_v).SubLib(v);
        }
    });
}

void RawBoard::remove_stone(Vertex v) {
    Player pl = color_at[v].ToPlayer();

    hash ^= zobrist->OfPlayerVertex(pl, v);
    player_v_cnt[pl]--;
    color_at[v] = Color::Empty();

    // TODO vector operations here would be a win.
    // TODO test if template wouldn't be more efficient
    hash3x3[v].ResetAtariBits();
    if (!tmp_vertex_set.IsMarked(v)) {
        hash3x3_changed.Push(v);
        tmp_vertex_set.Mark(v);
    }

    FOREACH_DIR(dir, {
        Vertex nbr = v.Nbr(dir);
        hash3x3[nbr].SetColorAt(dir.Opposite(), Color::Empty());
        if (!tmp_vertex_set.IsMarked(nbr) && color_at[nbr] == Color::Empty()) {
            hash3x3_changed.Push(nbr);
            tmp_vertex_set.Mark(nbr);
        }
    });

    empty_pos[v] = empty_v_cnt;
    empty_v[empty_v_cnt++] = v;
    chain_id[v] = v;

    vertex_for_each_4_nbr(v, nbr_v, nbr_cnt[nbr_v].player_dec(pl));

    ASSERT(empty_v_cnt < Vertex::kBound);
}

// TODO/FIXME last_player should be preserverd in undo function
Player RawBoard::ActPlayer() const { return last_player.Other(); }

void RawBoard::SetActPlayer(Player pl) { last_player = pl.Other(); }

Player RawBoard::LastPlayer() const { return last_player; }

Vertex RawBoard::LastVertex() const { return last_play[LastPlayer()]; }

Vertex RawBoard::LastVertex2() const { return last_play[ActPlayer()]; }

Move RawBoard::LastMove() const { return Move(LastPlayer(), LastVertex()); }

Move RawBoard::LastMove2() const {
    Player pl = ActPlayer();
    return Move(pl, last_play[pl]);
}

bool RawBoard::BothPlayerPass() const {
    return (last_play[Player::Black()] == Vertex::Pass()) &
           (last_play[Player::White()] == Vertex::Pass());
}

int RawBoard::TrompTaylorScore() const {  // TODO make it efficient
    NatMap<Player, int> score(0);

    ForEachNat(Player, pl) {
        FastStack<Vertex, kArea> queue;
        NatMap<Vertex, bool> visited(false);

        ForEachNat(Vertex, v) {
            if (color_at[v] == Color::OfPlayer(pl)) {
                queue.Push(v);
                visited[v] = true;
            }
        }

        while (!queue.IsEmpty()) {
            Vertex v = queue.PopTop();
            ASSERT(visited[v]);
            score[pl] += 1;
            vertex_for_each_4_nbr(v, nbr, {
                if (!visited[nbr] && color_at[nbr] == Color::Empty()) {
                    queue.Push(nbr);
                    visited[nbr] = true;
                }
            });
        }
    }
    return komi_inverse + score[Player::Black()] - score[Player::White()];
}

Player RawBoard::TrompTaylorWinner() const {
    return Player::WinnerOfBoardScore(TrompTaylorScore());
}

int RawBoard::StoneScore() const {
    return komi_inverse + player_v_cnt[Player::Black()] - player_v_cnt[Player::White()];
}

int RawBoard::EyeScore(Vertex v) const {
    return nbr_cnt[v].player_cnt_is_max(Player::Black()) -
           nbr_cnt[v].player_cnt_is_max(Player::White());
}

int RawBoard::PlayoutScore() const {
    int eye_score = 0;
    empty_v_for_each(this, v, eye_score += EyeScore(v));
    return StoneScore() + eye_score;
}

Player RawBoard::StoneWinner() const { return Player::WinnerOfBoardScore(StoneScore()); }

Player RawBoard::PlayoutWinner() const { return Player::WinnerOfBoardScore(PlayoutScore()); }

Vertex RawBoard::ChainIdAt(Vertex v) const { return chain_id[v]; }

// -----------------------------------------------------------------------------

void RawBoard::check_chain_atari_v() const {
    ForEachNat(Vertex, v) {
        if (!v.IsOnBoard()) continue;
        if (color_at[v] == Color::Empty()) continue;
        Vertex correct_av;
        if (chain_at(v).IsInAtari()) {
            correct_av = chain_at(v).AtariVertex();
        } else {
            correct_av = Vertex::Any();
        }
        CHECK2(chain_at(v).atari_v == correct_av, Dump1(v); WW(correct_av.ToGtpString());
               WW(chain_at(v).atari_v.ToGtpString()););
    }
}

void RawBoard::check_hash3x3() const {
    ForEachNat(Vertex, v) {
        if (!v.IsOnBoard()) continue;
        if (color_at[v] != Color::Empty()) continue;
        Hash3x3 correct_hash = Hash3x3::OfBoard(color_at, v);
        correct_hash.SetAtariBits(color_at[v.N()].IsPlayer() && chain_at(v.N()).IsInAtari(),
                                  color_at[v.E()].IsPlayer() && chain_at(v.E()).IsInAtari(),
                                  color_at[v.S()].IsPlayer() && chain_at(v.S()).IsInAtari(),
                                  color_at[v.W()].IsPlayer() && chain_at(v.W()).IsInAtari());
        CHECK2(hash3x3[v] == correct_hash, {
            Dump1(v);
            cerr << hash3x3[v].ToString() << " == " << correct_hash.ToString() << endl;
        });
    }
}

void RawBoard::check_empty_v() const {
    if (!kCheckAsserts) return;

    NatMap<Vertex, bool> noticed(false);
    NatMap<Player, uint> exp_player_v_cnt(0);

    ASSERT(empty_v_cnt <= kArea);

    empty_v_for_each(this, v, {
        ASSERT(noticed[v] == false);
        noticed[v] = true;
    });

    ForEachNat(Vertex, v) {
        ASSERT((color_at[v] == Color::Empty()) == noticed[v]);
        if (color_at[v] == Color::Empty()) {
            ASSERT(empty_pos[v] < empty_v_cnt);
            ASSERT(empty_v[empty_pos[v]] == v);
        }
        if (color_at[v].IsPlayer()) exp_player_v_cnt[color_at[v].ToPlayer()]++;
    }

    ForEachNat(Player, pl) ASSERT(exp_player_v_cnt[pl] == player_v_cnt[pl]);
}

void RawBoard::check_hash() const { ASSERT(hash == recalc_hash()); }

void RawBoard::check_color_at() const {
    if (!kCheckAsserts) return;

    ForEachNat(Vertex, v) { ASSERT((color_at[v] != Color::OffBoard()) == (v.IsOnBoard())); }
}

void RawBoard::check_nbr_cnt() const {
    if (!kCheckAsserts) return;

    ForEachNat(Vertex, v) {
        NatMap<Color, uint> nbr_color_cnt(0);
        if (color_at[v] == Color::OffBoard()) continue;  // TODO is that right?

        vertex_for_each_4_nbr(v, nbr_v, { nbr_color_cnt[color_at[nbr_v]]++; });

        nbr_cnt[v].check(nbr_color_cnt);
    }
}

void RawBoard::check_chain_at() const {
    if (!kCheckAsserts) return;

    ForEachNat(Vertex, v) {
        // whether same color neighbours have same root and liberties
        // TODO what about off_board and empty?
        if (color_at[v].IsPlayer()) {
            ASSERT(!chain_at(v).IsCaptured());

            vertex_for_each_4_nbr(v, nbr_v, {
                if (color_at[v] == color_at[nbr_v]) ASSERT(chain_id[v] == chain_id[nbr_v]);
            });
        }
    }
}

void RawBoard::check_chain_next_v() const {
    if (!kCheckAsserts) return;
    ForEachNat(Vertex, v) {
        // TODO chain_next_v[v].check ();
        if (!color_at[v].IsPlayer()) ASSERT(chain_next_v[v] == v);
    }
}

void RawBoard::check() const {
    if (!kCheckAsserts) return;

    check_empty_v();
    check_hash();
    check_color_at();
    check_nbr_cnt();
    check_chain_at();
    check_chain_next_v();
    check_hash3x3();
    check_chain_atari_v();
}

void RawBoard::check_no_more_legal(Player player) const {  // at the end of the playout
    unused(player);

    if (!kCheckAsserts) return;

    ForEachNat(Vertex, v) ASSERT(IsLegal(player, v) == false || IsEyelike(player, v));
}

const Zobrist RawBoard::zobrist[1] = {Zobrist()};

#undef vertex_for_each_4_nbr
#undef vertex_for_each_diag_nbr

// -----------------------------------------------------------------------------

void Board::Clear() {
    RawBoard::Clear();
    moves.clear();
}

void Board::Load(const Board& save_board) {
    RawBoard::Load(save_board);
    moves = save_board.moves;
}

void Board::PlayLegal(Player pl, Vertex v) {
    ASSERT(IsLegal(pl, v));
    moves.push_back(Move(pl, v));
    RawBoard::PlayLegal(pl, v);
}

void Board::PlayLegal(Move m) {
    ASSERT(IsLegal(m));
    moves.push_back(m);
    RawBoard::PlayLegal(m);
}

bool Board::Undo() {
    ASSERT(MoveCount() == moves.size());
    if (MoveCount() == 0) return false;

    vector<Move> replay = moves;
    Clear();

    rep(ii, replay.size() - 1) PlayLegal(replay[ii]);

    return true;
}

bool Board::IsReallyLegal(Move move) const {
    if (IsLegal(move) == false) return false;

    // Pass would repeat the hash.
    if (move.GetVertex() == Vertex::Pass()) return true;

    // Check for superko.
    Board tmp;
    tmp.Load(*this);
    tmp.PlayLegal(move);
    return !tmp.IsHashRepeated();
}

bool Board::IsHashRepeated() {
    RawBoard tmp_board;
    rep(mn, MoveCount() - 1) {
        tmp_board.PlayLegal(moves[mn]);
        if (PositionalHash() == tmp_board.PositionalHash()) return true;
    }
    return false;
}

const vector<Move>& Board::Moves() const { return moves; }

NetworkFeatures Board::getFeatures() const {
    const Player act = ActPlayer();

    NetworkFeatures features(act);

    for (uint r = 0; r < board_size; ++r) {
        for (uint c = 0; c < board_size; ++c) {
            const Vertex v = Vertex::OfCoords(r, c);
            features.colors[r][c] = ColorAt(v);
        }
    }

    static const uint m = std::numeric_limits<uint>::max();
    for (uint r = 0; r < board_size; ++r) {
        for (uint c = 0; c < board_size; ++c) {
            features.moves[r][c] = m;
        }
    }

    const uint mc = MoveCount();
    uint mi = 0;
    for (Move const& move : Moves()) {
        const Vertex v = move.GetVertex();
        features.moves[v.GetRow()][v.GetColumn()] = mc - mi;
        ++mi;
    }

    return features;
}

template <typename Planes>
uint addAsFeaturePlane(Planes& planes, uint f, const uint feature[board_size][board_size]) {
    static const uint numPlanes = 8;
    for (uint plane = 1; plane < numPlanes; ++plane) {
        for (uint r = 0; r < board_size; ++r) {
            for (uint c = 0; c < board_size; ++c) {
                planes[f][r][c] = feature[r][c] == plane;
            }
        }
        ++f;
    }

    for (uint r = 0; r < board_size; ++r) {
        for (uint c = 0; c < board_size; ++c) {
            planes[f][r][c] = feature[r][c] >= numPlanes;
        }
    }
    ++f;

    return f;
}

NetworkFeatures::Planes NetworkFeatures::getPlanes() const {
    Planes planes;

    for (uint r = 0; r < board_size; ++r) {
        for (uint c = 0; c < board_size; ++c) {
            uint f = 0;

            planes[f][r][c] = colors[r][c] == Color::Black();
            ++f;

            planes[f][r][c] = colors[r][c] == Color::White();
            ++f;

            planes[f][r][c] = colors[r][c] == Color::Empty();
            ++f;

            planes[f][r][c] = p == Player::White();
            ++f;

            f = addAsFeaturePlane(planes, f, moves);

            ASSERT(f == NUM_FEATURES);
        }
    }

    return planes;
}
