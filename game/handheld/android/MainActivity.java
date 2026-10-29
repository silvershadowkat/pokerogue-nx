// SPDX-License-Identifier: AGPL-3.0-only
package com.silvershadow.pokerogue.handheld;

import android.app.*;
import android.os.*;
import android.content.*;
import android.content.res.AssetFileDescriptor;
import android.net.Uri;
import android.view.*;
import android.webkit.*;
import android.widget.Toast;
import android.util.Base64;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.zip.InflaterInputStream;

public class MainActivity extends Activity {
 private WebView web;
 private final Map<String,Entry> index=new HashMap<>();
 private long packStart;
 private ValueCallback<Uri[]> fileCallback;
 private byte[] pendingExport;
 private static final String ORIGIN="https://pokerogue.local/";
 private static class Entry {long offset;int length,raw;boolean deflated;Entry(String[] f){offset=Long.parseLong(f[1]);length=Integer.parseInt(f[2]);raw=Integer.parseInt(f[3]);deflated=f[4].equals("1");}}
 private static class Slice extends InputStream {
  final RandomAccessFile file;long left;
  Slice(String path,long offset,long length)throws IOException {file=new RandomAccessFile(path,"r");file.seek(offset);left=length;}
  public int read()throws IOException {if(left<=0)return -1;int v=file.read();if(v>=0)left--;return v;}
  public int read(byte[] b,int off,int len)throws IOException {if(left<=0)return -1;int n=file.read(b,off,(int)Math.min(left,len));if(n>0)left-=n;return n;}
  public long skip(long n)throws IOException {long move=Math.min(left,n);file.seek(file.getFilePointer()+move);left-=move;return move;}
  public void close()throws IOException {file.close();}
 }
 @Override public void onCreate(Bundle state){
  super.onCreate(state);
  getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  immersive();
  try {
   try(AssetFileDescriptor fd=getAssets().openFd("game.pak")){packStart=fd.getStartOffset();}
   try(BufferedReader in=new BufferedReader(new InputStreamReader(getAssets().open("game.index"),StandardCharsets.UTF_8))){String line;while((line=in.readLine())!=null){String[] f=line.split("\t");index.put(f[0],new Entry(f));}}
  }catch(Exception e){new AlertDialog.Builder(this).setTitle("Asset pack could not open").setMessage(e.toString()).setPositiveButton("Close",(d,w)->finish()).show();return;}
  web=new WebView(this);web.setBackgroundColor(0xff101923);setContentView(web);
  WebSettings s=web.getSettings();s.setJavaScriptEnabled(true);s.setDomStorageEnabled(true);s.setMediaPlaybackRequiresUserGesture(false);
  s.setAllowFileAccess(false);s.setAllowContentAccess(true);s.setSupportZoom(false);s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
  // Debuggable is a test-build choice; the bridge exposes no arbitrary filesystem access.
  WebView.setWebContentsDebuggingEnabled(true);
  web.addJavascriptInterface(new Bridge(),"Android");
  web.setWebViewClient(new WebViewClient(){
   @Override public WebResourceResponse shouldInterceptRequest(WebView view,WebResourceRequest request){return asset(request);}
   @Override public boolean shouldOverrideUrlLoading(WebView view,WebResourceRequest request){
    if(request.getUrl().toString().startsWith(ORIGIN))return false;
    if(request.isForMainFrame()&&Arrays.asList("https","http").contains(request.getUrl().getScheme())){
     try{startActivity(new Intent(Intent.ACTION_VIEW,request.getUrl()));}catch(Exception ignored){}
    }return true;
   }
   @Override public void onPageFinished(WebView view,String url){android.util.Log.i("PokeRogueHandheld","Page loaded: "+url);}
   @Override public boolean onRenderProcessGone(WebView view,RenderProcessGoneDetail detail){
    view.destroy();web=null;
    new AlertDialog.Builder(MainActivity.this).setTitle("Game renderer stopped").setMessage("Your last saved wave is retained. Close and reopen the game.").setPositiveButton("Close",(d,w)->finish()).show();return true;
   }
  });
  web.setWebChromeClient(new WebChromeClient(){
   @Override public boolean onConsoleMessage(ConsoleMessage msg){if(msg.messageLevel()==ConsoleMessage.MessageLevel.ERROR)android.util.Log.e("PokeRogueJS",msg.message()+" at "+msg.sourceId()+":"+msg.lineNumber());return true;}
   @Override public boolean onShowFileChooser(WebView view,ValueCallback<Uri[]> callback,FileChooserParams params){
    if(fileCallback!=null)fileCallback.onReceiveValue(null);fileCallback=callback;
    try{Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*");startActivityForResult(i,10);}
    catch(Exception e){fileCallback.onReceiveValue(null);fileCallback=null;}return true;
   }
  });
  web.loadUrl(ORIGIN);web.requestFocus();
 }
 private WebResourceResponse asset(WebResourceRequest request){
  Uri uri=request.getUrl();
  if(!"https".equals(uri.getScheme())||!"pokerogue.local".equals(uri.getHost()))return response(403,"Forbidden","text/plain",new ByteArrayInputStream(new byte[0]),0,null);
  String path=uri.getPath();if(path==null||path.equals("/"))path="index.html";else path=path.substring(1);
  path=path.replaceAll("/+","/");
  if(path.contains("..")||path.contains("\\"))return response(403,"Forbidden","text/plain",new ByteArrayInputStream(new byte[0]),0,null);
  try {
   InputStream input;long length;Entry e=index.get(path);
   try{input=getAssets().open("web/"+path);length=input.available();}
   catch(IOException missing){if(e==null)throw missing;input=new Slice(getApplicationInfo().sourceDir,packStart+e.offset,e.length);if(e.deflated)input=new InflaterInputStream(input);length=e.raw;}
   String range=request.getRequestHeaders().get("Range");
   if(range==null)range=request.getRequestHeaders().get("range");
   if(range!=null&&range.matches("bytes=\\d+-\\d*")){
    String[] parts=range.substring(6).split("-",-1);long start=Long.parseLong(parts[0]),end=parts[1].isEmpty()?length-1:Math.min(length-1,Long.parseLong(parts[1]));
    if(start>=length||end<start){input.close();return response(416,"Range Not Satisfiable",mime(path),new ByteArrayInputStream(new byte[0]),0,"bytes */"+length);}
    long skip=start;while(skip>0){long n=input.skip(skip);if(n<=0){if(input.read()<0)throw new EOFException();n=1;}skip-=n;}
    final InputStream source=input;final long count=end-start+1;
    InputStream limited=new FilterInputStream(source){long left=count;public int read()throws IOException{if(left<=0)return -1;int n=super.read();if(n>=0)left--;return n;}public int read(byte[] b,int o,int l)throws IOException{if(left<=0)return -1;int n=super.read(b,o,(int)Math.min(left,l));if(n>0)left-=n;return n;}};
    return response(206,"Partial Content",mime(path),limited,count,"bytes "+start+"-"+end+"/"+length);
   }
   return response(200,"OK",mime(path),input,length,null);
  }catch(Exception error){android.util.Log.w("PokeRogueAsset","Missing: "+path);return response(404,"Not Found","text/plain",new ByteArrayInputStream(new byte[0]),0,null);}
 }
 private WebResourceResponse response(int code,String reason,String mime,InputStream data,long length,String range){
  Map<String,String> headers=new HashMap<>();headers.put("Content-Length",Long.toString(length));headers.put("Cache-Control","no-cache");headers.put("Accept-Ranges","bytes");
  if(range!=null)headers.put("Content-Range",range);
  return new WebResourceResponse(mime,"UTF-8",code,reason,headers,data);
 }
 private String mime(String p){String ext=p.substring(p.lastIndexOf('.')+1).toLowerCase(Locale.ROOT);switch(ext){case "html":return "text/html";case "js":case "mjs":return "application/javascript";case "css":return "text/css";case "json":return "application/json";case "png":return "image/png";case "svg":return "image/svg+xml";case "webp":return "image/webp";case "ogg":return "audio/ogg";case "mp3":return "audio/mpeg";case "m4a":return "audio/mp4";case "ttf":return "font/ttf";case "woff2":return "font/woff2";default:return "application/octet-stream";}}
 public class Bridge {
  @JavascriptInterface public void exportFile(String name,String base64){
   if(base64.length()>32000000)return;
   runOnUiThread(()->{if(pendingExport!=null)return;try{pendingExport=Base64.decode(base64,Base64.DEFAULT);Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("application/octet-stream").putExtra(Intent.EXTRA_TITLE,name.replaceAll("[^a-zA-Z0-9._-]","_"));startActivityForResult(i,11);}catch(Exception e){pendingExport=null;}});
  }
 }
 @Override protected void onActivityResult(int code,int result,Intent data){super.onActivityResult(code,result,data);
  if(code==10&&fileCallback!=null){fileCallback.onReceiveValue(result==RESULT_OK&&data!=null?new Uri[]{data.getData()}:null);fileCallback=null;}
  if(code==11&&pendingExport!=null){if(result==RESULT_OK&&data!=null)try(OutputStream out=getContentResolver().openOutputStream(data.getData())){out.write(pendingExport);Toast.makeText(this,"Save exported",Toast.LENGTH_SHORT).show();}catch(Exception e){Toast.makeText(this,"Export failed",Toast.LENGTH_LONG).show();}pendingExport=null;}
 }
 private void immersive(){getWindow().getDecorView().setSystemUiVisibility(5894);}
 @Override public void onWindowFocusChanged(boolean focused){super.onWindowFocusChanged(focused);if(focused)immersive();}
 @Override public void onBackPressed(){if(web!=null)web.evaluateJavascript("window.handheldBack?.()",null);}
 @Override protected void onPause(){if(web!=null){web.evaluateJavascript("window.handheldPause?.()",null);web.onPause();}super.onPause();}
 @Override protected void onResume(){super.onResume();if(web!=null){web.onResume();web.evaluateJavascript("window.handheldResume?.()",null);}}
 @Override protected void onDestroy(){if(fileCallback!=null)fileCallback.onReceiveValue(null);if(web!=null)web.destroy();super.onDestroy();}
}
