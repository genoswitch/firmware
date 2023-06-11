// SPDX-License-Identifier: MIT

const universalHex = require("@microbit/microbit-universal-hex");

const fs = require("fs");

const flashloaderFilePath = process.argv[2];
const appFilePath = process.argv[3];
const outputPath = process.argv[4];

if (flashloaderFilePath && appFilePath && outputPath) {
	console.log(`Using flashloader-only .hex at: '${flashloaderFilePath}'`);
	console.log(`Using app-only .hex at: '${appFilePath}'`);
	console.log(`Using '${outputPath}' as the output path.`);

	const flashloaderFile = fs.readFileSync(flashloaderFilePath, "utf-8");
	const appFile = fs.readFileSync(appFilePath, "utf-8");

	//const flashloader = universalHex.iHexToCustomFormatSection(flashloaderFile, 0x0);
	//const app = universalHex.iHexToCustomFormatSection(appFile, 0x1);

	const merged = universalHex.createUniversalHex([
		{ boardId: 0xffa0, hex: flashloaderFile },
		{ boardId: 0xffa1, hex: appFile },
	]);

	fs.writeFileSync(outputPath, merged);
} else {
	console.error("Missing arguments.");
	console.error("Usage ./[exec] <flashloader file path> <app file path> <output path>");
}

//universalHex.createUniversalHex()
