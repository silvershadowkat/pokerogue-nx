// Test vectors from the same crypto-js dependency as the official game.
import CryptoJS from 'crypto-js';
import fs from 'node:fs';
const vectors=['','hello','0123456789abcdef','Pokémon: 日本語 🎮', 'x'.repeat(4097)].map(message=>{
 const pass='PokeRogue-test-🔑',salt=CryptoJS.enc.Hex.parse('0001020304050607');
 const r=CryptoJS.AES.encrypt(message,pass,{salt});
 return {message,pass,key:r.key.toString(),iv:r.iv.toString(),ct:r.ciphertext.toString()};
});
fs.writeFileSync(process.argv[2],JSON.stringify(vectors));
