package com.lx281.netty.handler;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerAdapter;
import io.netty.channel.ChannelHandlerContext;

public class SimpleEchoHandler extends ChannelHandlerAdapter {
	@Override
	public void channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
		ByteBuf b = (ByteBuf) msg;
		while (b.isReadable()) {
			System.out.print((char) b.readByte());
			System.out.flush();
		}
		b.writeChar('a'); // 在msg后追加信息
		b.writeChar('\n');
		ctx.write(msg);
		ctx.flush();
		ctx.write("123");
	}

	@Override
	public void channelReadComplete(ChannelHandlerContext ctx) throws Exception {
		ctx.flush();
	}
}
