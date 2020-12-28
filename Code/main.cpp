#include <deque>
#include <set>

#include "Engine.h"

struct Tile {
	RenderObject renderObject;
	Engine::Vec2i posA, posB;
	char dir;
};

bool next = true;
float t = 0;

GL_Texture2D *block1;

std::set<Tile*> tiles;
std::map<int, std::map<int, Tile*>> tileMap;

std::deque<std::deque<char>> board;

void update(Engine::EngineContext &ctx);

Tile *createTile(int x, int y, char direction, int color);

std::deque<std::deque<char>> copyBoard(std::deque<std::deque<char>> board);
std::deque<std::deque<char>> copyBoardSize(std::deque<std::deque<char>> board);

VertexArrayObject masterVAO(VAO_UVS);

void growBoard();

void preUpdateBoard(Engine::EngineContext &ctx);

void updateBoard(Engine::EngineContext &ctx);

void moveTileRender(Tile &tile, float distance);

void moveTilePos(Tile &tile);

void removeTileAt(int i, int j, Engine::EngineContext &ctx);

void printBoard();

int main(int argc, char *args[]) {
	double fps = 16.0;
	if (argc > 1) fps = atof(args[1]);
	Engine::EngineContext ctx(fps, fps);
	Engine::initEngine(ctx);

	block1 = new GL_Texture2D("arrow_bock.png", GL_NEAREST);
	block1->generateMipMaps();
	block1->use();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);

	Shader mainShader;
	mainShader.buildShader("vertexshader.txt", "unlitFrag.txt");
	ctx.mainShader = mainShader.ID;

	Camera5DoF camera(ctx.mainWindow);
	ctx.mainCamera = &camera;
	camera.orthogonal = true;
	camera.setClipPlanes(-2000, 2000);
	camera.FOV = 0;

	ctx.events.insert({ SDL_EventType::SDL_KEYDOWN, [](Engine::EngineContext &ctx, SDL_Event event) {
		if (!next && event.key.keysym.sym == SDLK_SPACE) {
			next = true;
			t = 0;
		}
	}});

	ctx.fixedUpdate.push_back(update);

	masterVAO.addQuadWithUVs({
		-1, -0.5, 0,
		0, 1,
		1, -0.5, 0,
		1, 1,
		1, 0.5, 0,
		1, 0,
		-1, 0.5, 0,
		0, 0
		});
	masterVAO.bindBuffers();

	srand(SDL_GetTicks());

	Engine::startMainGameLoop(ctx, true);

	return 0;
}

void update(Engine::EngineContext &ctx) {
	static int size = 0;
	if (next) {
		if (t == 0) {
			growBoard();
			preUpdateBoard(ctx);
		}
		if (t < 1) {
			float delta = 1.0f / (ctx.tickLimiter.getStaticUPS() * 1.0f);
			t += delta;
			ctx.mainCamera->FOV += delta;
			for (Tile *tile : tiles) {
				moveTileRender(*tile, delta);
			}
		}
		else {
 			next = false;
			size++;
			for (Tile *tile : tiles) {
				moveTilePos(*tile);
			}
			updateBoard(ctx);
		}
	}
}

Tile *createTile(int x, int y, char direction, int color) {
	Tile *tile = new Tile();
	tile->renderObject.renderContext.blendEnable = false;
	tile->renderObject.renderContext.texture = block1;
	tile->renderObject.renderContext.cullEnable = false;
	tile->renderObject.VAO = masterVAO;
	if (direction == 1) {
		tile->posA = { x + 1, y + 1 };
		tile->posB = { x + 1, y };
		tile->renderObject.transform.m.rotateZd(90).translate(x + 0.5f, y, (float)color);
	}
	else if (direction == 2) {
		tile->posA = { x + 1, y + 1 };
		tile->posB = { x, y + 1 };
		tile->renderObject.transform.m.translate(x, y + 0.5f, (float)color);
	}
	else if (direction == 3) {
		tile->posA = { x, y };
		tile->posB = { x, y + 1 };
		tile->renderObject.transform.m.rotateZd(-90).translate(x - 0.5f, y, (float)color);
	}
	else  if (direction == 4) {
		tile->posA = { x, y };
		tile->posB = { x + 1, y };
		tile->renderObject.transform.m.rotateZd(180).translate(x, y - 0.5f, (float)color);
	}
	tile->dir = direction;
	return tile;
}

std::deque<std::deque<char>> copyBoard(std::deque<std::deque<char>> board) {
	int size = board.size();
	std::deque<std::deque<char>> newBoard;
	for (int i = 0; i < size; i++) {
		newBoard.push_back(std::deque<char>());
		for (int j = 0; j < size; j++) {
			newBoard[i].push_back(board[i][j]);
		}
	}
	return newBoard;
}

std::deque<std::deque<char>> copyBoardSize(std::deque<std::deque<char>> board) {
	std::deque<std::deque<char>> newBoard;
	int size = board.size();
	for (int i = 0; i < size; i++) {
		newBoard.push_back(std::deque<char>());
		for (int j = 0; j < size; j++) {
			newBoard[i].push_back(board[i][j] == -1 ? -1 : 0);
		}
	}
	return newBoard;
}

void growBoard() {
	int size = board.size();
	int size2 = size + 2;
	std::deque<char> row1;
	row1.insert(row1.begin(), size, -1);
	std::deque<char> row2;
	row2.insert(row2.begin(), size, -1);
	board.push_front(row1);
	board.push_back(row2);
	for (int i = 0; i < size2; i++) {
		board[i].push_front(-1);
		board[i].push_back(-1);
	}
	for (int i = 0; i < size2 * 2; i++) {
		int s = size2 - 1;
		int i2 = i % size2;
		int j = abs(s / 2 - (i2 * s + s / 2) / size2);
		int k1 = (i / size2);
		int k2 = k1 * 2 - 1;
		board[i2][k1 * s - k2 * j] = 0;
	}
}

void preUpdateBoard(Engine::EngineContext & ctx) {
	int size = board.size();
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (board[i][j] > 0) {
				switch (board[i][j]) {
				case 1:
					if (board[i + 1][j] == 3) {
						board[i][j] = 0;
						board[i + 1][j] = 0;
						removeTileAt(i - size / 2 + 1, j - size / 2 + 1, ctx);
						removeTileAt(i - size / 2 + 2, j - size / 2 + 1, ctx);
					}
					break;
				case 2:
					if (board[i][j + 1] == 4) {
						board[i][j] = 0;
						board[i][j + 1] = 0;
						removeTileAt(i - size / 2 + 1, j - size / 2 + 1, ctx);
						removeTileAt(i - size / 2 + 1, j - size / 2 + 2, ctx);
					}
					break;
				case 3:
					if (board[i - 1][j] == 1) {
						board[i][j] = 0;
						board[i - 1][j] = 0;
						removeTileAt(i - size / 2 + 1, j - size / 2 + 1, ctx);
						removeTileAt(i - size / 2, j - size / 2 + 1, ctx);
					}
					break;
				case 4:
					if (board[i][j - 1] == 2) {
						board[i][j] = 0;
						board[i][j - 1] = 0;
						removeTileAt(i - size / 2 + 1, j - size / 2 + 1, ctx);
						removeTileAt(i - size / 2 + 1, j - size / 2, ctx);
					}
				}
			}
		}
	}
}

void updateBoard(Engine::EngineContext &ctx) {
	const int size = board.size();
	std::deque<std::deque<char>> newBoard = copyBoardSize(board);
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (board[i][j] > 0) {
				switch (board[i][j]) {
				case 1:
					newBoard[i + 1][j] = board[i][j];
					break;
				case 2:
					newBoard[i][j + 1] = board[i][j];
					break;
				case 3:
					newBoard[i - 1][j] = board[i][j];
					break;
				case 4:
					newBoard[i][j - 1] = board[i][j];
				}
				board[i][j] = 0;
			}
		}
	}
	board = copyBoard(newBoard);
	for (int i = 0; i < size - 1; i++) {
		for (int j = 0; j < size - 1; j++) {
			if (board[i][j] == 0 && board[i + 1][j] == 0 && board[i][j + 1] == 0 && board[i + 1][j + 1] == 0) {
				int i1 = i - size / 2 + 1;
				int j1 = j - size / 2 + 1;
				if (rand() & 1) {
					board[i][j] = board [i][j + 1] = 3;
					board[i + 1][j] = board[i + 1][j + 1] = 1;
					Tile *right = createTile(i1, j1, 1, 7);
					Tile *left = createTile(i1, j1, 3, 7 | (7 << 3));
					tileMap[i1][j1] = left;
					tileMap[i1][j1 + 1] = left;
					tileMap[i1 + 1][j1] = right;
					tileMap[i1 + 1][j1 + 1] = right;
					tiles.insert(right);
					tiles.insert(left);
					Engine::addMesh(ctx, right->renderObject);
					Engine::addMesh(ctx, left->renderObject);
				}
				else {
					board[i][j] = board[i + 1][j] = 4;
					board[i][j + 1] = board[i + 1][j + 1] = 2;
					Tile *top = createTile(i1, j1, 2, 7 << 6);
					Tile *bottom = createTile(i1, j1, 4, 7 << 3);
					tileMap[i1][j1] = bottom;
					tileMap[i1][j1 + 1] = top;
					tileMap[i1 + 1][j1] = bottom;
					tileMap[i1 + 1][j1 + 1] = top;
					tiles.insert(top);
					tiles.insert(bottom);
					Engine::addMesh(ctx, top->renderObject);
					Engine::addMesh(ctx, bottom->renderObject);
				}
			}
		}
	}
}

void moveTileRender(Tile & tile, float distance) {
	if (tile.dir == 1) {
		tile.renderObject.transform.m.translateX(distance);
	}
	else if (tile.dir == 2) {
		tile.renderObject.transform.m.translateY(distance);
	}
	else if (tile.dir == 3) {
		tile.renderObject.transform.m.translateX(-distance);
	}
	else if (tile.dir == 4) {
		tile.renderObject.transform.m.translateY(-distance);
	}
}

void moveTilePos(Tile & tile) {
	if (tileMap[tile.posA.x][tile.posA.x] == &tile) tileMap[tile.posA.x].erase(tile.posA.y);
	if (tileMap[tile.posB.x][tile.posB.x] == &tile) tileMap[tile.posB.x].erase(tile.posB.y);
	if (tile.dir == 1) {
		tile.posA = { tile.posA.x + 1, tile.posA.y };
		tile.posB = { tile.posB.x + 1, tile.posB.y };
	}
	else if (tile.dir == 2) {
		tile.posA = { tile.posA.x, tile.posA.y + 1 };
		tile.posB = { tile.posB.x, tile.posB.y + 1 };
	}
	else if (tile.dir == 3) {
		tile.posA = { tile.posA.x - 1, tile.posA.y };
		tile.posB = { tile.posB.x - 1, tile.posB.y };
	}
	else if (tile.dir == 4) {
		tile.posA = { tile.posA.x, tile.posA.y - 1 };
		tile.posB = { tile.posB.x, tile.posB.y - 1 };
	}
	tileMap[tile.posA.x][tile.posA.y] = &tile;
	tileMap[tile.posB.x][tile.posB.y] = &tile;
}

void removeTileAt(int i, int j, Engine::EngineContext &ctx) {
	if (tileMap[i].find(j) != tileMap[i].end()) {
		Tile *tile = tileMap[i][j];
		tileMap[tile->posA.x].erase(tile->posA.y);
		tileMap[tile->posB.x].erase(tile->posB.y);
		ctx.renderObjects.erase(std::find(ctx.renderObjects.begin(), ctx.renderObjects.end(), &(tile->renderObject)));
		tiles.erase(tile);
		delete(tile);
	}
}

void printBoard() {
	int size = board.size();
	for (int i = size - 1; i >= 0; i--) {
		for (int j = 0; j < size; j++) {
			switch (board[j][i]) {
			case -1:
				printf(" ");
				break;
			case 0:
				printf("O");
				break;
			case 1:
				printf(">");
				break;
			case 2:
				printf("A");
				break;
			case 3:
				printf("<");
				break;
			case 4:
				printf("V");
			}
		}
		puts("");
	}
	puts("");
}
