#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <thread>
#include "ai.hpp"
#include "positions.hpp"

// g++ -I /usr/include/boost -pthread websocket.cpp 
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    system("chcp 65001 > nul");

        auto const address = net::ip::make_address("127.0.0.1");
        auto const port = static_cast<unsigned short>(std::atoi("8083"));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        for(;;)
        {

            tcp::socket socket{ioc};

            acceptor.accept(socket);

            std::thread{std::bind(
                //[q = std::move(socket)]() mutable { // socket will be const - mutable should be used
                [q{std::move(socket)}]() { // socket will be const - mutable should be used
                     
                websocket::stream<tcp::socket> ws{std::move(const_cast<tcp::socket&>(q))};
                    
                constexpr U8 Set = Chess5D::NoPiece;
                constexpr U8 Size = 8;
                constexpr U16 L = 32;
                constexpr U16 T = 128;
            
                constexpr bool White = true;
                
                Chess5D::Chess<Set, Size, L, T> chess{};
                Positions::load(chess, 0);

                // Set a decorator to change the Server of the handshake
                // no need to set. It ıs not necessary
                ws.set_option(websocket::stream_base::decorator(
                    [](websocket::response_type& res)
                    {
                        res.set(http::field::server,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-server-sync");
                                }));

                        // Accept the websocket handshake
                        ws.accept();

                        while(true) {
                            try {
                                beast::flat_buffer buffer;
                                ws.read(buffer);
                                auto message = beast::buffers_to_string(buffer.cdata());
                                std::cout << message << std::endl;

                                //Split move by " " and loop
                                std::vector<std::string> moves;
                                std::stringstream ss(message);
                                std::string move;
                                
                                auto trim = [](std::string& s) {
                                    s.erase(0, s.find_first_not_of(" \n\r\t"));
                                    s.erase(s.find_last_not_of(" \n\r\t") + 1);
                                };
                                
                                while (std::getline(ss, move, ' ')) {
                                    trim(move);
                                    if (move.empty()) continue;
                                    std::cout << "Move: " << move << std::endl;
                                    chess.template makeMove<true>(chess.PGNtoMove<true>(move));
                                }
                                std::cout << chess << std::endl;

                                Chess5D::killerTable.empty();
                                tt.clear(); // TODO: Was causing issues with detecting mates, but it shouldn't be necessary to clear it every time. Should be able to keep TT for usein aiding searches
                                // Prompt user for max depth
                                int depth=21;
                                bool isBlack = true;
                                
                                count = 0;
                                hitCount = 0;
                                collision = 0;
                                auto begin = std::chrono::high_resolution_clock::now();
                                // Chess5D::Result res = isBlack ? negaMax<Set, Size, L, T, false>(chess, -CHECKMATE, CHECKMATE, depth, std::vector<Chess5D::Move>()) : negaMax<Set, Size, L, T, true>(chess, -CHECKMATE, CHECKMATE, depth, std::vector<Chess5D::Move>());
                                std::unique_ptr<Chess5D::Result>  res = isBlack ? negaMaxIDDFS<Set, Size, L, T, false>(chess, depth, std::chrono::milliseconds(1000)) : negaMaxIDDFS<Set, Size, L, T, true>(chess, depth, std::chrono::milliseconds(1000));
                                //std::unique_ptr<Chess5D::Result>  res = isBlack ? negaMax<Set, Size, L, T, false, true>(chess, -CHECKMATE, CHECKMATE, 2, 0, std::vector<Chess5D::Move>()) : negaMax<Set, Size, L, T, true, true>(chess, -CHECKMATE, CHECKMATE, 2, 0, std::vector<Chess5D::Move>()) ;

                                auto end = std::chrono::high_resolution_clock::now();
                                std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1000000000.0 << " s\n";
                                std::cout << "Positions: " << count << std::endl;
                                std::cout << "Hits: " << hitCount << std::endl;
                                std::cout << "Collision: " << collision << std::endl;
                                std::cout << chess << std::endl;
                                std::cout << res->value << std::endl;
                                
                                int timeline = chess.origIndex[!isBlack];
                                std::cout<<"Backtrace:"<< std::endl; 
                                isBlack ? backtrace<Set, Size, L, T, false>(chess, timeline, 0) : backtrace<Set, Size, L, T, true>(chess, timeline, 0);
                                
                                std::cout << (int)res->moveset.size()<< std::endl;
                                std::vector<std::string> movePGNs;
                                for(std::size_t i = 0; i < res->moveset.size(); ++i){
                                    std::string movePGN=chess.moveToPGN<false>(res->moveset[i]);
                                    movePGNs.push_back(movePGN);
                                    std::cout << movePGN << std::endl;
                                    chess.template makeMove<false>(res->moveset[i]);
                                }
                                std::string payload = "[\"" + 
                                    std::accumulate(std::next(movePGNs.begin()), movePGNs.end(), movePGNs[0],
                                        [](const std::string& a, const std::string& b) { return a + "\",\"" + b; }) 
                                    + "\"]";

                                ws.write(net::buffer(payload));

                            } catch(beast::system_error const& se) {
                                if(se.code() != websocket::error::closed) {
                                    std::cerr << "Error: " << se.code().message() << std::endl;
                                }
                                break;
                            }
                        }                        
                }
            )}.detach();
        }
}